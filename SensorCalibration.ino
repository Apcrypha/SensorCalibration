//-------------------------------------WiFi--------------------------
#include <WiFi.h>

WiFiClient  client;

const char* ssid = "Teenage Nigga Turtles";    
const char* password = "Nigga_Bazooka";   

//-------------------------------------Thingspeak-----------------
#include "ThingSpeak.h" //Thingspeak by mathworks

unsigned long gyroChannel_ID = 3498647;
const char * gyroWriteAPIKey = "W6I9KNG5O1F41071";

unsigned long lastUploadTime = 0;
uint16_t uploadInterval = 16000; //in ms


//-------------------------------------RRH62000-------------------
#include "RRH62000.h"

#define RRH_SDA 21
#define RRH_SCL 22

RRH62000 RRH_sensor;


//-------------------------------------Gyro--------------------------
#include <Wire.h>
#include "FastIMU.h" //by LiquidCGS

#define IMU_ADDRESS 0x68  // Set to 0x69 if AD0 is tied to 3.3V
#define SDA_PIN 21
#define SCL_PIN 22

MPU6500 IMU;              // Create MPU6500 FastIMU instance
calData calib = { 0 };    // Calibration struct (zero-initialized if uncalibrated)
AccelData accelData;
GyroData gyroData;


void setup() {
  Serial.begin(115200); 

//RRH62000  
  if (!RRH_sensor.begin(RRH_SDA, RRH_SCL)) {
    Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
    while (1);
    }

//Gyro
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);  // 400 kHz Fast I2C bus

  // Initialize MPU6500
  int err = IMU.init(calib, IMU_ADDRESS);
  if (err != 0) {
    Serial.print("Error initializing MPU6500. Code: ");
    Serial.println(err);
    while (true);
  }

  // Set measurement range limits
  IMU.setAccelRange(8);   // Options: 2, 4, 8, 16 (g)
  IMU.setGyroRange(500);  // Options: 250, 500, 1000, 2000 (deg/s)

//WiFi
  WiFi.mode(WIFI_STA); 
  
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect");
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, password); 
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }

//Thingspeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak

}

void loop() {
unsigned long currentMillis = millis();

//ThingSpeak
  if (currentMillis - lastUploadTime >= uploadInterval) {
    lastUploadTime = currentMillis;

    //Ensure WiFi is connected
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Connecting.....");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password); 
        delay(5000);     
      } 
      Serial.println("\nConnected.");
    }

    //Gyro
    IMU.update();
    IMU.getAccel(&accelData);
    IMU.getGyro(&gyroData);
    
    // set the fields with the values
    ThingSpeak.setField(1,accelData.accelX);
    ThingSpeak.setField(2,accelData.accelY);
    ThingSpeak.setField(3,accelData.accelZ);
    ThingSpeak.setField(4,gyroData.gyroX);
    ThingSpeak.setField(5,gyroData.gyroY);
    ThingSpeak.setField(6,gyroData.gyroZ);

    int x = ThingSpeak.writeFields(gyroChannel_ID, gyroWriteAPIKey);
    if(x == 200){Serial.println("Gyro Channel update successful.");    }
    else{Serial.println("Problem updating gyro channel. HTTP error code " + String(x));    }
  }


  if (RRH_sensor.readSensor()) {

    //Must measure Temp, RH, PM10 & PM2.5 (KCl & Smoke)

    // Read directly from library member variables
    Serial.printf("PM2.5 (KCl): %.1f ug/m3 | PM2.5 (Smoke): %.1f ug/m3\n", RRH_sensor.pm2_5_kcl, RRH_sensor.pm2_5_smoke);
    Serial.printf("PM10 (KCl): %.1f ug/m3 | PM10 (Smoke): %.1f ug/m3\n", RRH_sensor.pm10_0_kcl, RRH_sensor.pm10_0_smoke);
    Serial.printf("Temp: %.2f C | Humidity: %.2f %%\n", RRH_sensor.temperature, RRH_sensor.humidity);
    Serial.printf("TVOC: %u ug/m3 | eCO2: %u ppm | IAQ: %.2f\n", RRH_sensor.tvoc, RRH_sensor.eco2, RRH_sensor.iaq);
    Serial.println("------------------------------------------------");
    } else {
      Serial.println("Failed to read sensor data or CRC mismatch.");
    }




}





