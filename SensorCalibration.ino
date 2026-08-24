#include <WiFi.h>
#include "ThingSpeak.h"

const char* ssid = "REPLACE_WITH_YOUR_SSID";   // your network SSID (name) 
const char* password = "REPLACE_WITH_YOUR_PASSWORD";   // your network password

WiFiClient  client;

//----------------------------------------Thingspeak-----------------
unsigned long myChannelNumber = 2;
const char * myWriteAPIKey = "XXXXXXXXXXXXXXXX";



#define TIA_address 0x90





void setup() {
  Serial.begin(92000);
  WiFi.mode(WIFI_STA);   
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

}

void loop() {
  // put your main code here, to run repeatedly:

}

void changeGain(){
/*
- TIA_address
- wait for ACK
- 0x01 = Targets the LOCK register
- wait for ACK
- 0x00 = unlock register
- wait for ACK
- Stop condition

- TIA_address
- wait for ACK
- 0x10 = Targets the TIACN register
- wait for ACK
- 0x-- = Specify the gain. Refer to the datasheet
- wait for ACK
- Stop condition

*/

}

void startTIA(){
/*
- TIA_address
- wait for ACK
- 0x12 = Targets the MODECN register
- wait for ACK
- 0 0000 011 = Enables analog reading from the Vout pin
- wait for ACK
- Stop condition

*/

}

void stopTIA(){
/*
- TIA_address
- wait for ACK
- 0x12 = Targets the MODECN register
- wait for ACK
- 1 0000 000 = Enables analog reading from the Vout pin
- wait for ACK
- Stop condition

*/

}
