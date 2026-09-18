//-------------------------------------WiFi--------------------------
#include <WiFi.h>

WiFiClient  client;

const char* ssid = "REPLACE_WITH_YOUR_SSID";   // your network SSID (name) 
const char* password = "REPLACE_WITH_YOUR_PASSWORD";   // your network password


//-------------------------------------Thingspeak-----------------
#include "ThingSpeak.h"

unsigned long myChannelNumber = 2;
const char * myWriteAPIKey = "XXXXXXXXXXXXXXXX";


//-------------------------------------RRH62000-------------------
#include "RRH62000.h"

#define RRH_SDA 21
#define RRH_SCL 22

RRH62000 RRH_sensor;


//-------------------------------------Gyro--------------------------
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;





void setup() {
  Serial.begin(92000);

//RRH62000  
  if (!RRH_sensor.begin(RRH_SDA, RRH_SCL)) {
    Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
    while (1);
    }

//Gyro
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // Options are 2, 4, 8, 16 g
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);      // Options are ± 250, 500, 1000, 2000 deg/s
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);   // Options are 5, 10, 21, 44 94, 184, 260 Hz

//WiFi
  WiFi.mode(WIFI_STA); 
  
//Thingspeak  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

}

void loop() {




//Gyro
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

}





