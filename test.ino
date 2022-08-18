
#include <heltec.h>
#include "Arduino.h"
#include <vector>
String serial2Out;
String serialIn;
#define BAND 866E6
const int intake = 21;
const int sensor_exhaust = 13;
const int filter_exhaust = 12;
const int plate = 39;

void oledSetup()
{
  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->setContrast(255);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
}

// the setup routine runs once when starts up
void setup()
{
  // Initialize serial
  Serial.begin(115200);

  //setting up Piera serial
  Serial2.begin(115200, SERIAL_8N1, 17, 23);

  //setting up fan pins
  pinMode(intake, OUTPUT);
  pinMode(sensor_exhaust, OUTPUT);
  pinMode(filter_exhaust, OUTPUT);
  pinMode(plate, OUTPUT);

  // Initialize the Heltec ESP32 object
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /**/);
  oledSetup();

  Heltec.display->drawString(0, 0, "Warming up 7100...");
  Heltec.display->display();
  Serial.println("Warming up 7100...");
  delay(5000);

  Heltec.display->drawString(0, 10, "Cleaning Sensor...");
  Heltec.display->display();
  Serial.println("Cleaning sensor...");
  Serial2.write("$Wcln=1\r\n");
  delay(3000);
  Serial2.write("Wcln=0\r\n");
  delay(100);
  Serial2.write("$Wfactory=\r\n");
  //delay(100);
  //Serial2.write("$Wdebug=1\r\n");
  delay(100);


  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(0, 0, "Starting measurements...");
  Heltec.display->display();
}

// the loop routine runs over and over again forever
void loop() {
  // Check serial data from sensor

  if (Serial2.available())
  {
    digitalWrite(25, LOW);
    
    serial2Out = Serial2.readStringUntil('\n');
    String serialString = serial2Out; //create string object

    Serial2.write("Wfan=0\r\n");
    /*delay(200);
    digitalWrite(sensor_exhaust,LOW);
    digitalWrite(intake, HIGH);
    digitalWrite(plate, HIGH);
    delay(1000);

    digitalWrite(intake, LOW);
    digitalWrite(filter_exhaust, HIGH);
    delay(1000);

    digitalWrite(filter_exhaust, LOW);
    delay(300);
    digitalWrite(plate, LOW);
    digitalWrite(sensor_exhaust, HIGH);*/
    Serial2.write("Wfan=1\r\n");
    

    LoRa.beginPacket();
    LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
    LoRa.setSpreadingFactor(7);
    LoRa.setCodingRate4(8);
    LoRa.setSignalBandwidth(250E3);
 
    LoRa.print(serial2Out);
    oledSetup();
    String amt = (String) serial2Out.length();
    Heltec.display->drawString(0,0,amt);
    Heltec.display->drawStringMaxWidth(0, 15, 128, serial2Out );
    Heltec.display->display();
    
    LoRa.endPacket();
    String fullOutput = serial2Out + "\r\n";
    if (serial2Out)
    {
      // Write sensor data to USB serial output'
      Serial.write(fullOutput.c_str());
    }
    
    digitalWrite(25, HIGH);
    //delay(100);
    
  }
}
