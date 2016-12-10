#include <Sodaq_RN2483.h>
#include <Sodaq_wdt.h>
#include <math.h>

//#define DEBUG

#ifdef ARDUINO_ARCH_AVR
#define debugSerial Serial
#define LED LED1
#elif ARDUINO_ARCH_SAMD
#define debugSerial SerialUSB
#define LED LED_BUILTIN
#endif

#define loraSerial Serial1

//global constants
const uint8_t
DEV_ADDR[4]  = {0x14, 0x00, 0xB1, 0xF0 },
               NWK_SKEY[16] = {0xd9, 0x03, 0x66, 0x08, 0xe0, 0x97, 0x80, 0x79, 0xf1, 0x93, 0xf7, 0xf1, 0xeb, 0x4c, 0xb6, 0x85},
                              APP_SKEY[16] = {0xcc, 0x8b, 0x40, 0xf2, 0xe3, 0xb5, 0x04, 0x11, 0x3b, 0xf8, 0xa9, 0x4c, 0xab, 0xfc, 0x66, 0x7b};

const int
WAKEUP_INTERVAL_S = 1800; //15 minutes

const bool
nl = true;

//globar vars
bool
softReset = false;

/****************************************************************************
  ...
*****************************************************************************/
void setup()
{
  sodaq_wdt_disable();                                             //disable watchdog to prevent startup loop after soft reset

  pinMode(LED, OUTPUT);                                            // Setup LED, on by default

  digitalWrite(BEE_VCC, HIGH);                                    //supply power to lora bee

#ifdef DEBUG
  while ((!debugSerial) && (millis() < 10000));                  //Wait for debugSerial or 10 seconds
  delay(10000);
  debugSerial.begin(9600);
  LoRaBee.setDiag(debugSerial);
#endif

  while (!loraSerial);
  loraSerial.begin(LoRaBee.getDefaultBaudRate());

  if (!LoRaBee.initABP(loraSerial, DEV_ADDR, APP_SKEY, NWK_SKEY))            //setup LORA connection
  {
    logMsg("Connection to the network failed!", nl);
    softReset = true;
  }
  else
  {
    logMsg("Connection to the network successful.", nl);
    setDataRate(0);                                                    //HACK because no acknoledgements received
  }
  onSetup();
  sodaq_wdt_enable(WDT_PERIOD_8X);                          //wakeup interval = 16s// Enable WDT
}

int sleepRemainingS = 0;

/****************************************************************************
  ...
*****************************************************************************/
void loop()
{
  sodaq_wdt_flag = false;                                 //mark wdt timeout as handled
  sleepRemainingS -= 16;                                 //substract wdt interval

  if (softReset)
  {
    static bool resetTriggered = false;
    if (!resetTriggered)                               //prevent code from executing multiple times
    {
      resetTriggered = true;
      logMsg("Soft reset triggered!", nl);
    }
    return;
  }
  else
  {
    sodaq_wdt_reset();
    digitalWrite(LED, HIGH);                         //start LED flash

    if (sleepRemainingS <= 0)
    {
      onWakeup();
      sleepRemainingS = WAKEUP_INTERVAL_S;
    }
    else
    {
      delay(100);
    }

    digitalWrite(LED, LOW);                             //end LED flash
  }
  sodaq_wdt_reset();

#ifdef DEBUG
  delay(8000); //instead of sleeping, delay with watchdog interval
#else
  systemSleep();
#endif
}

/****************************************************************************
  ...
*****************************************************************************/
void setDataRate(int dr)
{
  loraSerial.print("mac set dr 0\r\n");
  String result = loraSerial.readString();
  logMsg("Setting data rate to " + String(dr) + ": " + result, nl);
}

/****************************************************************************
  ...
*****************************************************************************/
void setBatteryLevel(int bat)
{
  //   loraSerial.print("mac set bat " + String(bat) + "\r\n");
  loraSerial.print("mac set bat 127\r\n");
  String result = loraSerial.readString();
  logMsg("Setting battery level to " + String(bat) + ": " + result, nl);
}

/****************************************************************************
  ...
*****************************************************************************/
void logMsg(String msg, bool nl)
{
#ifdef DEBUG
  if (nl)
  {
    debugSerial.println(msg);
  }
  else
  {
    debugSerial.print(msg);
  }
#endif
}

/****************************************************************************
  Sleep until WDT interrupt
  code used from the documentation in avr/sleep.h
*****************************************************************************/
void systemSleep()
{
#ifdef DEBUG
  debugSerial.flush();
#endif

#ifdef ARDUINO_ARCH_AVR
  ADCSRA &= ~_BV(ADEN);                        // ADC disabled

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  cli();

  if (!sodaq_wdt_flag)                         // Only go to sleep if there was no watchdog interrupt.
  {
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
  sei();

  ADCSRA |= _BV(ADEN);                        // ADC enabled

#elif ARDUINO_ARCH_SAMD
  if (!sodaq_wdt_flag)                           // Only go to sleep if there was no watchdog interrupt.
  {
    USBDevice.detach();                           // Disable USB
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;                 // Set the sleep mode
    __WFI();                                          // SAMD sleep
  }
#endif
}
