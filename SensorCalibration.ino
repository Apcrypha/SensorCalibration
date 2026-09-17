#include <WiFi.h>
#include "ThingSpeak.h"
#include "RRH62000.h"

//-------------------------------------WiFi--------------------------

WiFiClient  client;

const char* ssid = "REPLACE_WITH_YOUR_SSID";   // your network SSID (name) 
const char* password = "REPLACE_WITH_YOUR_PASSWORD";   // your network password



//----------------------------------------Thingspeak-----------------

unsigned long myChannelNumber = 2;
const char * myWriteAPIKey = "XXXXXXXXXXXXXXXX";


//----------------------------------------RRH62000-------------------
#define RRH_SDA 21
#define RRH_SCL 22

RRH62000 RRH_sensor;


void setup() {
  Serial.begin(92000);
  WiFi.mode(WIFI_STA);   
  
  if (!RRH_sensor.begin(RRH_SDA, RRH_SCL)) {
        Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
        while (1);
    }
  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

}

void loop() {



}





