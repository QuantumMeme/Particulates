
#include "heltec.h"
#include "Arduino.h"
#include "images.h"

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6


String outgoing;              // outgoing message

byte localAddress = 0xBB;     // address of this device
//byte destination = 0xFF;      // destination to send to
/*
 * receiver address: 0xBB
 * transmitter 1 address: 0xFD
 * transmitter 2 address: 0xC5
 */

byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
int interval = 200;          // interval between sends
String serialMessage;

void displayMessage(String message)
{
  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(0, 0, message);
  Heltec.display->display();
}

void logo(){
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,logo_width,logo_height,logo_bits);
  Heltec.display->display();
}

void LoRaData(String packet, String addr){
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0 , 0 , "Received from "+ addr);
  Heltec.display->drawStringMaxWidth(0 , 15 , 128, packet); 
  Heltec.display->display();
}
void setup()
{
   //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Serial.begin(115200, SERIAL_8N1, 17, 23);
  Serial.println("Heltec.LoRa Duplex");
  
  Heltec.display->init();
  //Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  logo();
  delay(1500);
  Heltec.display->clear();
  
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 10, "Wait for incoming data...");
  Heltec.display->display();
  delay(1000);
  LoRa.receive();
  //LoRa.setSignalBandwidth(250E3);
 
}

void loop()
{
  
  /*  if (millis() - lastSendTime > interval)
  {
    String message = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";   // send a message
    sendMessage(message);
    Serial.println("Sending " + message);
    displayMessage(message);
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 1000;    // 2-3 seconds
  }
  */

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}


void onReceive(int packetSize)
{
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.setSpreadingFactor(10);
  LoRa.setCodingRate4(8);
  if (packetSize == 0){
    //Serial.println("No message received");
    return;          // if there's no packet, return
  }

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();

  LoRaData(incoming, String(sender,HEX));

  serialMessage = String(sender,HEX) + "," + incoming;
  Serial.println(serialMessage);
  Serial.write(serialMessage.c_str());
}
