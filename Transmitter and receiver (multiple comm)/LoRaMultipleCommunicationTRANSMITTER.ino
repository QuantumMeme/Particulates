
#include "heltec.h"
#include "Arduino.h"

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6


String outgoing;              // outgoing message
String serial2Out;
String serialIn;
byte localAddress = 0xFD;     // address of this device
byte destination = 0xBB;      // destination to send to
/*
 * receiver address: 0xBB
 * transmitter 1 address: 0xFD
 * transmitter 2 address: 0xC5
 */

byte msgCount = 0;            // count of outgoing messages
long lastSendTime = 0;        // last send time
int interval = 200;          // interval between sends

void displayMessage(String message)
{
  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(0, 0, message);
  Heltec.display->display();
}

void oledSetup()
{
  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->setContrast(255);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
}

void setup()
{
   //WIFI Kit series V1 not support Vext 
  Serial.begin(115200);

  //setting up Piera serial
  Serial2.begin(115200, SERIAL_8N1, 17, 23);
  Serial.println("Heltec.LoRa Duplex");

  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
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
  //Serial2.write("$Wfactory=\r\n");
  //delay(100);
  Serial2.write("$Winterval=30\r\n");
  //Serial2.write("$Won=200\r\n");
  delay(500);


  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(0, 0, "Starting measurements...");
  Heltec.display->display();
}

void loop()
{ 
  if (Serial2.available())
  {
    //digitalWrite(25, HIGH);
    
    serial2Out = Serial2.readStringUntil('\n');
    String serialString = serial2Out; //create string object
    //LoRa.setSignalBandwidth(250E3);
    
    oledSetup();
    String amt = (String) serial2Out.length();
    Heltec.display->drawString(0,0,amt);
    Heltec.display->drawStringMaxWidth(0, 15, 128, serial2Out );
    Heltec.display->display();
    
    sendMessage(serial2Out);


    String fullOutput = serial2Out + "\r\n";
    if (serial2Out)
    {
      // Write sensor data to USB serial output'
      Serial.write(fullOutput.c_str());
    }
    digitalWrite(25, LOW);
    //delay(100);
  // parse for a packet, and call onReceive with the result:
  //onReceive(LoRa.parsePacket());
  }
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // start packet
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);
  LoRa.setSpreadingFactor(10);
  LoRa.setCodingRate4(8);
  
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}


}
