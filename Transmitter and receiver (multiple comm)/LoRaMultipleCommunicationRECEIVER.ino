
#include "heltec.h" 
#include "images.h"
#include "Arduino.h"

#define BAND 915E6  //you can set band here directly,e.g. 868E6,915E6
String packet ;

void logo(){
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,logo_width,logo_height,logo_bits);
  Heltec.display->display();
}

void LoRaData(String packSize){
  Heltec.display->clear();
  Heltec.display->drawString(0 , 0 , "Received "+ packSize + " bytes");
  Heltec.display->drawStringMaxWidth(0 , 15 , 128, packet);  
  Heltec.display->display();
}

void onReceive(int packetSize)
{
  //packet = "";
  for (int i = 0; i < packetSize; i++) { /*packet += (char) LoRa.read();*/ Serial.print((char)LoRa.read()); }
  Serial.println();
  //packet = packet + "\n";
  //Serial.print(packet.c_str());
  //LoRaData((String) packetSize);
}

void setup() { 
   //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);

  //opening serial to communicate w/ raspberry pi
  Serial.begin(115200, SERIAL_8N1, 17, 23); 
 
  Heltec.display->init();
  //Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  logo();
  delay(1500);
  
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");
  Heltec.display->display();
  delay(1000);
  LoRa.onReceive(onReceive);
  LoRa.setSpreadingFactor(7);
  LoRa.setCodingRate4(8);
  LoRa.setSignalBandwidth(250E3);

  LoRa.receive();
}

void loop()
{

  }
