#include "heltec.h"
#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6
int counter = 0;
String packet;

int SENSOR_ID = 5;

const int intake = 0;
const int sensor_exhaust = 2;
const int filter_exhaust = 22;
const int plate = 21;

void setup() {
  pinMode(intake, OUTPUT);
  pinMode(sensor_exhaust, OUTPUT);
  pinMode(filter_exhaust, OUTPUT);
  pinMode(plate, OUTPUT);
  //local serial
  Serial.begin(115200);
  //setting up Piera serial
  Serial2.begin(115200, SERIAL_8N1, 17, 23);
  Serial.print("Serial open!");
  
  // setting up routines on the ESP32
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);


  Heltec.display->clear();
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

  //setting up the timing for the sensor
  //Serial2.write("$Wfactory=\r\n");
  //delay(100);
  Serial2.write("$Winterval=15\r\n");
  Serial2.write("$Wfan=1\r\n");
  //Serial2.write("$Won=200\r\n");
  delay(3000);

  //LoRa specific settings

  /*
* LoRa.setTxPower(txPower,RFOUT_pin);
* txPower -- 0 ~ 20
* RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
*   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
*   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
 */
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.setSpreadingFactor(7);
  LoRa.setCodingRate4(8);
  LoRa.setSignalBandwidth(250E3);

  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(0, 0, "Starting measurements...");
  Heltec.display->display();
  
}

void loop() {

  Serial.print("GETTING SAMPLE\n");
  digitalWrite(filter_exhaust, LOW);
  digitalWrite(intake,HIGH);
  digitalWrite(plate, HIGH);
  delay(5000);

  Serial.print("CLEARING SMALLER PARTICULATE\n");
  digitalWrite(intake, LOW);
  digitalWrite(filter_exhaust, HIGH);
  delay(5000);

  Serial.print("TAKING MEASUREMENTS\n");
  digitalWrite(filter_exhaust, LOW);
  Serial2.write("$Wfan=1\r\n");
  delay(1000);
  digitalWrite(sensor_exhaust, HIGH);
  digitalWrite(plate, LOW);
  delay(1000);

  
  //Declaring empty string to store packet
  int flag = 0;
  while(flag == 0)
  {
      if (Serial2.available())
      {
      packet = (String) SENSOR_ID + ",";
      // clear display
      Heltec.display->clear();
      Heltec.display->display();
      
      packet = packet + Serial2.readStringUntil('\n');
      
      int amt = packet.length();
      Serial.print(packet + "\n");
  
      Heltec.display->drawString(0,0,(String) amt);
      Heltec.display->drawStringMaxWidth(0, 15, 128, packet );
      Heltec.display->display();
      if (amt > 185)
      {
        LoRa.beginPacket();
        LoRa.print(packet.c_str());
        LoRa.endPacket();
        flag = 1;
      }
      else
      {
        Serial.print("Packet too small, not sending.\n");
      }
    }
  }

  Serial.print("CLEARING CHAMBER\n");  
  digitalWrite(sensor_exhaust, LOW);
  Serial2.write("$Wfan=0\r\n");
  digitalWrite(filter_exhaust, HIGH);
  delay(3000);

}
