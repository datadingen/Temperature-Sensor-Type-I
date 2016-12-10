/****************************************************************************
  This is a build of the eDevice for measuring the ambient temperature in a
  room. This is also called the referencetemparature.

  This software is designed specifically to work with the ... Sodaq Autonomo
  and LoRabee LoRa shield.

  //http://downloads.sodaq.net/package_sodaq_index.json

  The LoRabee shield communicates with the Arduino via SPI. The SPI uses for
  communication the pins:

  - 13 = SCK
  - 12 *= MISO
  - 11 = MOSI

  Author: Richard Cuperus
  Date:   05-11-2016
  Version: 0.1.0001
  Revisions:
  - initial version

*****************************************************************************/
//Headers
#include <Sodaq_RN2483.h>
#include <Sodaq_wdt.h>--
//#include <avr/dtostrf.h>

#define debugSerial SerialUSB
#define loraSerial  Serial1

//t.b.v. temp.meter
const int
B = 4299,               // B value of the thermistor
R0 = 100000,           // R0 = 100k
pinTempSensor = A5;     // Grove - Temperature Sensor connect to A5

const byte
VERSION = 0;

char
payload[4];                                                             //Variable to hold the payload for LoRaWAN (9*0)

bool
firstRun = true;

/****************************************************************************
  ...
*****************************************************************************/
void onSetup()
{
  for (int i = 0; i < sizeof(payload); i++)              //fill payload with 0xFF
  {
    payload[i] = 0xFF;
  }
}

/****************************************************************************
  After waking up the temperature must be requested en send via LoRaWAN
*****************************************************************************/
void onWakeup()
{
  //   SerialUSB.println("Requesting data...");

  setBatteryLevel(127);                                                    //Set the batterylevel

  int a = analogRead(pinTempSensor );
  float R = 1023.0 / ((float)a) - 1.0;
  R = 100000.0 * R;
  long temperature = 100.0 / (log(R / 100000.0) / B + 1 / 298.15) - 273.15;        //convert to temperature via datasheet ;

  for (int i = 0; i <= 3; i++)                                          //construct payload from temperature
  {
    char c = 0xFF & temperature >> 8 * i;
    payload[3 - i] = c;
  }

  sendMsg();
  delay(2000);
}

/****************************************************************************
  ...
*****************************************************************************/
void sendMsg()
{
  LoRaBee.send(1, (uint8_t*)payload, sizeof(payload));
}
