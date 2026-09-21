//-------------------------------------WiFi-----------------------
#include <WiFi.h>

WiFiClient  client;

const char* ssid = "Teenage Nigga Turtles";    
const char* password = "Nigga_Bazooka";   

//-------------------------------------Thingspeak-----------------
#include "ThingSpeak.h" //Thingspeak by mathworks

unsigned long gyroChannel_ID = 3498647;
const char * gyroWriteAPIKey = "W6I9KNG5O1F41071";

unsigned long PM_Channel_ID = 1111;
const char * PM_WriteAPIKey = "xxxxx";

unsigned long statusChannel_ID = 1111;
const char * statusWriteAPIKey = "xxxxx";

int uploadStatus;


//-------------------------------------RRH62000-------------------
#include "RRH62000.h"

#define RRH_SDA 21
#define RRH_SCL 22

#define samplingTime 180000 //Sampling interval should be (MovingAverage * 3) * 1000

RRH62000 RRH_sensor;


//-------------------------------------Gyro-----------------------
#include <Wire.h>
#include "FastIMU.h" //by LiquidCGS

#define IMU_ADDRESS 0x68  // Set to 0x69 if AD0 is tied to 3.3V
#define SDA_PIN 21
#define SCL_PIN 22

MPU6500 IMU;              // Create MPU6500 FastIMU instance
calData calib = { 0 };    // Calibration struct (zero-initialized if uncalibrated)
AccelData accelData;
GyroData gyroData;

//-------------------------------------Sleep----------------------
#include "esp_wifi.h"
#include "esp_sleep.h"

const uint64_t sleepTime = 173ULL * 1000000ULL; // Must be in ms. Must use Unsigned Long Long(ULL) since its in 64bit. Format is (seconds * ms/s)
//due to wifi reasons reduce the actual sleep time with 7seconds for thingspeak

void setup() {
  Serial.begin(115200); 

//RRH62000  
  if (!RRH_sensor.begin(RRH_SDA, RRH_SCL)) {
    Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
    while (!RRH_sensor.begin(RRH_SDA, RRH_SCL)){
      Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
      delay(1000);
    }
  }

  // Configure module parameters via I2C
  RRH_sensor.setMovingAverage(60);          // Sampling interval should be MovingAverage * 3 
  RRH_sensor.setCleaningInterval(2880);     // Set auto-cleaning interval (2880 * 30s = 24 hours)
  RRH_sensor.setCleaningTime(15);           // Run fan cleaning for 15 seconds
  RRH_sensor.setFanSpeed(70);               // Set fan speed to 70%

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
  
  Serial.print("Attempting to connect");
  while(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password); 
    delay(5000);     
  } 
  Serial.println("\nConnected.");
  
//Thingspeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}


void loop() {
//Ensure WiFi is connected
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Reconnecting..");
    WiFi.reconnect();
    delay(5000);     
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

  uploadStatus = ThingSpeak.writeFields(gyroChannel_ID, gyroWriteAPIKey); 
  while ( uploadStatus != 200){
    Serial.println("Problem updating gyro channel. HTTP error code " + String(uploadStatus));
    WiFi.reconnect();
    delay(1000);
    uploadStatus = ThingSpeak.writeFields(gyroChannel_ID, gyroWriteAPIKey);
  }
  Serial.println("Gyro Channel update successful.");

//PM    
  if (RRH_sensor.readSensor()) {
    ThingSpeak.setField(1,RRH_sensor.temperature);
    ThingSpeak.setField(2,RRH_sensor.humidity);
    ThingSpeak.setField(3,RRH_sensor.pm10_0_kcl);
    ThingSpeak.setField(4,RRH_sensor.pm10_0_smoke);
    ThingSpeak.setField(5,RRH_sensor.pm2_5_kcl);
    ThingSpeak.setField(6,RRH_sensor.pm2_5_smoke);

    uploadStatus = ThingSpeak.writeFields(gyroChannel_ID, gyroWriteAPIKey);
    while (uploadStatus != 200){
      Serial.println("Problem updating PM channel. HTTP error code " + String(uploadStatus));
      WiFi.reconnect();
      delay(1000);
      uploadStatus = ThingSpeak.writeFields(gyroChannel_ID, gyroWriteAPIKey);
    }
    Serial.println("PM Channel update successful.");

   //Status
    if (RRH_sensor.status_fan_malfunction) {ThingSpeak.setField(7, 1);} //This will be sent to the next channel which is gyro 
      
    if (RRH_sensor.status_dust_accumulation) {
      RRH_sensor.triggerManualCleaning();
    }
  }
//Sleep
  delay(1000); //Add delay to make sure serial printing is done
  client.stop();  //Close TCP connection

  esp_sleep_enable_timer_wakeup(sleepTime);
    
  // Pause CPU execution while keeping Wi-Fi alive
  esp_light_sleep_start(); 
  // Execution resumes directly HERE after sleeping

  //Allow WiFi handshake to rerun
  delay(7000); 

}





