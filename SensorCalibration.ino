//#define RRH_debug 
#define Complete_run
//#define Complete_serial


//-------------------------------------WiFi-----------------------
#include <WiFi.h>

WiFiClient  client;

#define SSID                        "MightBreadboard"    //House WiFi: Teenage Nigga Turtles   | Pocket WiFi: MightBreadboard
#define PASSWORD                    "Mighty_Breadboard"        //House WiFi: Nigga_Bazooka           | Pocket WiFi: Mighty_Breadboard

//-------------------------------------Thingspeak-----------------
#include "ThingSpeak.h" //Thingspeak by mathworks

#define STATUS_CHANNEL_ID           3498647U
#define STATUS_WRITE_API_KEY        "W6I9KNG5O1F41071"

#define PM_CHANNEL_ID               3506848U
#define PM_WRITE_API_KEY            "ZDEWLBAPYNDSP98U"

int uploadStatus;

uint8_t systemStatus = 0;  
/*  
******Bit masking for Air monitoring Status******
        | BIT    |       MEANING        |
        |  0     |   System Capsized    |
        |  1     |   Fan Malfunction    |
        |  2     |         -            |
        |  3     |         -            |
        |  4     |         -            |
        |  5     |         -            |
        |  6     |         -            |
        |  7     |         -            |
*/
//-------------------------------------RRH62000-------------------
#include "RRH62000.h"

#define SDA_PIN                     21
#define SCL_PIN                     22

#define RRH_SAMPLING_TIME           (180 * 1000) //in seconds. 1,000 is seconds to milliseconds conversion. Sampling interval should be (MovingAverage * 3)

RRH62000 RRH_sensor;

//-------------------------------------Gyro-----------------------
#include <Wire.h>
#include "FastIMU.h" //by LiquidCGS

#define IMU_ADDRESS                 0x68  // Set to 0x69 if AD0 is tied to 3.3V

MPU6500 IMU;              // Create MPU6500 FastIMU instance
calData calib = { 0 };    // Calibration struct (zero-initialized if uncalibrated)
AccelData accelData;
GyroData gyroData;

#define GYRO_DELTA_TIME             0.02f //in seconds
#define GYRO_SAMPLING_TIME          ((unsigned long)(GYRO_DELTA_TIME  * 1000000)) // must be in microseconds. 1,000,000 is seconds to microseconds 


// Filtered angle variables
float pitch = 0.0;  //X axis in degrees
float roll = 0.0;   //Y axis in degrees

//in degrees
#define TILT_THRESHOLD              50.0f

//-------------------------------------Timing---------------------
unsigned long lastSampleMicros = 0; //For gyro
unsigned long lastSampleMillis = 0; //For PM
unsigned long lastStatusMillis = 0; //For status

#define STATUS_INTERVAL             (16 * 1000) //in seconds. the interval on status update. 1,000 is seconds to milliseconds conversion


//-------------------------------------Status LED-----------------
#define PM_LED_Sent_Ok              14  //Green
#define PM_LED_Sent_Err             13  //Red
#define RRH_LED_Disconnected        18  //Blue

#define Status_LED_Sent_Ok          17  //Green
#define Status_LED_Sent_Err         16  //Red

#define Gyro_LED_Disconnected       19  //Red
#define WiFi_LED_Disconnected       4   //Blue

//-----------------------------------------------------------------------------------------------------------Complete Run------------------------------------------------------------------------------

#ifdef Complete_run
  void setup() {
    Serial.begin(115200); 
    Serial.println("\n\n\n------------Complete Set------------");

  //RRH62000  
    if (!RRH_sensor.begin(SDA_PIN, SCL_PIN)) {
      Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
      while (!RRH_sensor.begin(SDA_PIN, SCL_PIN)){
        digitalWrite(RRH_LED_Disconnected, HIGH);
        Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
        delay(1000);
      }
    }

    // Configure module parameters via I2C
    RRH_sensor.setMovingAverage(60);          // Sampling interval should be MovingAverage * 3 
    RRH_sensor.setCleaningInterval(2880);     // Set auto-cleaning interval (2880 * 30s = 24 hours)
    RRH_sensor.setCleaningTime(30);           // Run fan cleaning for 15 seconds
    RRH_sensor.setFanSpeed(70);               // Set fan speed to 70%

    digitalWrite(RRH_LED_Disconnected, LOW);
    Serial.println("RRH62000 Working");

  //Gyro
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);  

    // Initialize MPU6500
    int err = IMU.init(calib, IMU_ADDRESS);
    while (err != 0) {
      digitalWrite(Gyro_LED_Disconnected, HIGH);
      Serial.print("Error initializing MPU6500. Code: ");
      Serial.println(err);
      err = IMU.init(calib, IMU_ADDRESS);
      delay(2000);
    }
    
    // Set measurement range limits
    IMU.setAccelRange(8);   // Options: 2, 4, 8, 16 (g)
    IMU.setGyroRange(500);  // Options: 250, 500, 1000, 2000 (deg/s)
    
    digitalWrite(Gyro_LED_Disconnected, LOW);
    Serial.println("IMU Working");

  //WiFi
    WiFi.mode(WIFI_STA); 
    
    Serial.print("Attempting to connect");
    while(WiFi.status() != WL_CONNECTED){
      digitalWrite(WiFi_LED_Disconnected, HIGH);
      WiFi.begin(SSID, PASSWORD); 
      delay(5000);     
    } 
    digitalWrite(WiFi_LED_Disconnected, LOW);
    Serial.println("\nConnected.");
    
  //Thingspeak
    ThingSpeak.begin(client);  // Initialize ThingSpeak

    delay(2000);
  }

  void loop() {

    unsigned long currentMicros = micros();
    unsigned long currentMillis = millis();

   //Ensure WiFi is connected
    while(WiFi.status() != WL_CONNECTED){
      digitalWrite(WiFi_LED_Disconnected, HIGH);
      Serial.println("Reconnecting..");
      WiFi.reconnect();
      delay(5000);     
    } 
    digitalWrite(WiFi_LED_Disconnected, LOW);

   //Gyro
    if(currentMicros - lastSampleMicros >= GYRO_SAMPLING_TIME ){
      lastSampleMicros = currentMicros;

      IMU.update();
      IMU.getAccel(&accelData);
      IMU.getGyro(&gyroData);
      
      // 1. Calculate Roll and Pitch from Accelerometer
      float accelRoll = atan2(accelData.accelY, accelData.accelZ) * 180.0 / M_PI;
      float accelPitch = atan2(-accelData.accelX, sqrt(accelData.accelY * accelData.accelY + accelData.accelZ * accelData.accelZ)) * 180.0 / M_PI;

      // 2. Complementary Filter
      // 96% Gyro integration + 4% Accelerometer anchor
      roll = 0.96 * (roll + gyroData.gyroX * GYRO_DELTA_TIME) + 0.04 * accelRoll;
      pitch = 0.96 * (pitch + gyroData.gyroY * GYRO_DELTA_TIME) + 0.04 * accelPitch;

      if(abs(roll) >= TILT_THRESHOLD || abs(pitch) >= TILT_THRESHOLD){
        systemStatus |= 1<<0; //Forces bit 0 to 1
      }
      else{
        systemStatus &= ~(1<<0); //Forces bit 0 to 0
      }
    }

   //PM
    if(currentMillis - lastSampleMillis >= RRH_SAMPLING_TIME  ){  //1,000 is seconds to milliseconds conversion
      lastSampleMillis = currentMillis;

      if (RRH_sensor.readSensor()) {
        ThingSpeak.setField(1,RRH_sensor.temperature);
        ThingSpeak.setField(2,RRH_sensor.humidity);
        ThingSpeak.setField(3,RRH_sensor.pm10_0_kcl);
        ThingSpeak.setField(4,RRH_sensor.pm10_0_smoke);
        ThingSpeak.setField(5,RRH_sensor.pm2_5_kcl);
        ThingSpeak.setField(6,RRH_sensor.pm2_5_smoke);
        ThingSpeak.setField(7,RRH_sensor.pm1_0_kcl);
        ThingSpeak.setField(8,RRH_sensor.pm1_0_smoke);

        uploadStatus = ThingSpeak.writeFields(PM_CHANNEL_ID, PM_WRITE_API_KEY);
        while (uploadStatus != 200){

          digitalWrite(PM_LED_Sent_Err, HIGH);
          Serial.print("Problem updating PM channel. HTTP error code ");
          Serial.println(uploadStatus);
          
          WiFi.reconnect();
          delay(3000);
          uploadStatus = ThingSpeak.writeFields(PM_CHANNEL_ID, PM_WRITE_API_KEY);
        }

        digitalWrite(PM_LED_Sent_Err, LOW);
        delay(50);
        digitalWrite(PM_LED_Sent_Ok, HIGH);
        delay(300);
        digitalWrite(PM_LED_Sent_Ok, LOW);

        Serial.println("PM Channel update successful.");

        if (RRH_sensor.status_fan_malfunction) {
          systemStatus |= 1 << 1; //Forces bit 1 to 1
        } 
        else{
          systemStatus &= ~(1 << 1);  //Forces bit 1 to 0
        }
        if (RRH_sensor.status_dust_accumulation) {
          RRH_sensor.triggerManualCleaning();
        }
      }
    }

   //Status
    if(systemStatus){ // != 0, wwhich means it has an error
      if(currentMillis - lastStatusMillis >= STATUS_INTERVAL ){ 
        lastStatusMillis = currentMillis;

        // set the fields with the values
        ThingSpeak.setField(1, systemStatus); 
        uploadStatus = ThingSpeak.writeFields(STATUS_CHANNEL_ID, STATUS_WRITE_API_KEY); 
        while ( uploadStatus != 200){
        digitalWrite(Status_LED_Sent_Err, HIGH);
        Serial.println("Problem updating Status channel: Field 1. HTTP error code " + String(uploadStatus));
        WiFi.reconnect();
        delay(3000);
        uploadStatus = ThingSpeak.writeFields(STATUS_CHANNEL_ID, STATUS_WRITE_API_KEY);
        }
        digitalWrite(Status_LED_Sent_Err, LOW);
        delay(50);
        digitalWrite(Status_LED_Sent_Ok, HIGH);
        delay(300);
        digitalWrite(Status_LED_Sent_Ok, LOW);
        Serial.println("Status Channel: Field 1 update successful.");
      }
    }
  }
#endif


//-----------------------------------------------------------------------------------------------------------RRH Debug------------------------------------------------------------------------------
//RRH with thingspeak

#ifdef RRH_debug
  void setup() {
    Serial.begin(115200); 
    Serial.println("\n\n\n------------RRH debug------------");

  //RRH62000  
    if (!RRH_sensor.begin(SDA_PIN, SCL_PIN)) {
      Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
      while (!RRH_sensor.begin(SDA_PIN, SCL_PIN)){
        Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
        delay(1000);
      }
    }

    // Configure module parameters via I2C
    RRH_sensor.setMovingAverage(60);          // Sampling interval should be MovingAverage * 3 
    RRH_sensor.setCleaningInterval(2880);     // Set auto-cleaning interval (2880 * 30s = 24 hours)
    RRH_sensor.setCleaningTime(15);           // Run fan cleaning for 15 seconds
    RRH_sensor.setFanSpeed(70);               // Set fan speed to 70%

    Serial.println("RRH62000 Working");

  //WiFi
    WiFi.mode(WIFI_STA); 
    
    Serial.print("Attempting to connect");
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(SSID, PASSWORD); 
      delay(5000);     
    } 
    Serial.println("\nConnected.");
    
  //Thingspeak
    ThingSpeak.begin(client);  // Initialize ThingSpeak
  }


  void loop() {
    unsigned long currentMicros = micros();
    unsigned long currentMillis = millis();

  //Ensure WiFi is connected
    while(WiFi.status() != WL_CONNECTED){
      Serial.println("Reconnecting..");
      WiFi.reconnect();
      delay(5000);     
    } 


    if(currentMillis - lastSampleMillis >= RRH_SAMPLING_TIME){
      lastSampleMillis = currentMillis;

      if (RRH_sensor.readSensor()) {
        ThingSpeak.setField(1,RRH_sensor.temperature);
        ThingSpeak.setField(2,RRH_sensor.humidity);
        ThingSpeak.setField(3,RRH_sensor.pm10_0_kcl);
        ThingSpeak.setField(4,RRH_sensor.pm10_0_smoke);
        ThingSpeak.setField(5,RRH_sensor.pm2_5_kcl);
        ThingSpeak.setField(6,RRH_sensor.pm2_5_smoke);

        uploadStatus = ThingSpeak.writeFields(PM_CHANNEL_ID, PM_WRITE_API_KEY);
        while (uploadStatus != 200){
          Serial.println("Problem updating PM channel. HTTP error code " + String(uploadStatus));
          WiFi.reconnect();
          delay(1000);
          uploadStatus = ThingSpeak.writeFields(PM_CHANNEL_ID, PM_WRITE_API_KEY);
        }
        Serial.println("PM Channel update successful.");

      //Status
        if (RRH_sensor.status_fan_malfunction) {
          ThingSpeak.setField(2, 1);//1 is broken fan, 0 is ok
          uploadStatus = ThingSpeak.writeFields(STATUS_CHANNEL_ID, STATUS_WRITE_API_KEY);
          while ( uploadStatus != 200){
            Serial.println("Problem updating Status channel: Field 2. HTTP error code " + String(uploadStatus));
            WiFi.reconnect();
            delay(1000);
            uploadStatus = ThingSpeak.writeFields(STATUS_CHANNEL_ID, STATUS_WRITE_API_KEY);
          }
          Serial.println("Status Channel: Field 2 update successful.");
        } 
          
        if (RRH_sensor.status_dust_accumulation) {
          RRH_sensor.triggerManualCleaning();
        }

      }
    }

  }

#endif

//-----------------------------------------------------------------------------------------------------------Complete Serial------------------------------------------------------------------------------
//Comple code but Serial only

#ifdef Complete_serial

  int tilt = 0; 
  void setup() {
    Serial.begin(115200); 
    Serial.println("\n\n\n------------Complete Serial------------");

  //RRH62000  
    if (!RRH_sensor.begin(SDA_PIN, SCL_PIN)) {
      Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
      while (!RRH_sensor.begin(SDA_PIN, SCL_PIN)){
        Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
        delay(1000);
      }
    }

    // Configure module parameters via I2C
    RRH_sensor.setMovingAverage(60);          // Sampling interval should be MovingAverage * 3 
    RRH_sensor.setCleaningInterval(2880);     // Set auto-cleaning interval (2880 * 30s = 24 hours)
    RRH_sensor.setCleaningTime(15);           // Run fan cleaning for 15 seconds
    RRH_sensor.setFanSpeed(70);               // Set fan speed to 70%

    Serial.println("RRH62000 Working");

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
    
    Serial.println("IMU Working");

    delay(2000);
  }


  void loop() {

    unsigned long currentMicros = micros();
    unsigned long currentMillis = millis();

  //Gyro
    if(currentMicros - lastSampleMicros >= GYRO_SAMPLING_TIME){
      lastSampleMicros = currentMicros;

      IMU.update();
      IMU.getAccel(&accelData);
      IMU.getGyro(&gyroData);
      
      // 1. Calculate Roll and Pitch from Accelerometer
      float accelRoll = atan2(accelData.accelY, accelData.accelZ) * 180.0 / M_PI;
      float accelPitch = atan2(-accelData.accelX, sqrt(accelData.accelY * accelData.accelY + accelData.accelZ * accelData.accelZ)) * 180.0 / M_PI;

      // 2. Complementary Filter
      // 96% Gyro integration + 4% Accelerometer anchor
      roll = 0.96 * (roll + gyroData.gyroX * GYRO_SAMPLING_TIME) + 0.04 * accelRoll;
      pitch = 0.96 * (pitch + gyroData.gyroY * GYRO_SAMPLING_TIME) + 0.04 * accelPitch;

      Serial.print("Pitch:"); Serial.print(pitch, 3); Serial.print(",");  Serial.print("Roll:"); Serial.print(roll, 3); Serial.print(","); Serial.print("Tilt:"); Serial.print(tilt); Serial.print(",");
      Serial.print("temp:"); Serial.print(RRH_sensor.temperature, 3); Serial.print(",");  Serial.print("RH:"); Serial.print(RRH_sensor.humidity, 3);  Serial.print(",");
      Serial.print("PM10KCL:"); Serial.print(RRH_sensor.pm10_0_kcl, 3); Serial.print(",");  Serial.print("PM10smk:"); Serial.print(RRH_sensor.pm10_0_smoke, 3); Serial.print(",");
      Serial.print("PM2.5KCL:"); Serial.print(RRH_sensor.pm2_5_kcl, 3); Serial.print(",");  Serial.print("PM2.5smk:"); Serial.println(RRH_sensor.pm2_5_smoke, 3);    

      if(abs(roll) >= rollThreshold || abs(pitch) >= pitchThreshold){
        tilt = 1;
      }
      else{
        tilt = 0;
      }
    }

  //PM
    if(currentMillis - lastSampleMillis >= RRH_SAMPLING_TIME){
      lastSampleMillis = currentMillis;

      RRH_sensor.readSensor();

    }
  }

#endif


