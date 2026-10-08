
//-------------------------------------WiFi------------------------
  #include <WiFi.h>

  WiFiClient  client;

  #define SSID                        "Mighty Breadboard"    //House WiFi: Teenage Nigga Turtles   | Pocket WiFi: Mighty Breadboard
  #define PASSWORD                    "We_are:Mighty-Breadboard"        //House WiFi: Nigga_Bazooka           | Pocket WiFi: We_are:Mighty-Breadboard

//-------------------------------------Thingspeak------------------
  #include "ThingSpeak.h" //Thingspeak by mathworks
  #include <HTTPClient.h>


  #define STATUS_CHANNEL_ID           3498647U
  #define STATUS_WRITE_API_KEY        "W6I9KNG5O1F41071"

  #define PM_CHANNEL_ID               3506848U
  #define PM_WRITE_API_KEY            "ZDEWLBAPYNDSP98U"

  //                                                                      TALKBACK_ID                      TALKBACK_API_KEY        
  #define TALKBACK_URL                "https://api.thingspeak.com/talkbacks/57954/commands/execute?api_key=3R1GNZ8OKY0ZIXYI"

  int uploadStatus;

  uint8_t systemStatus = 0;  
  /*  
  ******Bit masking for Air monitoring Status******
          | BIT    |                MEANING               |             DEFINITION
          |  0     |            System Capsized           |
          |  1     |            Fan Malfunction           |
          |  2     |    Extreme particle concentration    |
          |  3     |         Restart Notification         |   Goes high whenever the esp32 restarts
          |  4     |                 -                    |
          |  5     |                 -                    |
          |  6     |                 -                    |
          |  7     |                 -                    |
  */
//-------------------------------------RRH62000--------------------
  #include "RRH62000.h"

  #define SDA_PIN                     21
  #define SCL_PIN                     22

  #define RRH_SAMPLING_TIME           (180 * 1000) //in seconds. 1,000 is seconds to milliseconds conversion. Sampling interval should be (MovingAverage * 3)

  RRH62000 RRH_sensor;

//-------------------------------------Gyro------------------------
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
  #define TILT_THRESHOLD              40.0f

//-------------------------------------Timing----------------------
  unsigned long lastSampleMicros = 0; //For gyro
  unsigned long lastSampleMillis = 0; //For PM
  unsigned long lastStatusMillis = 0; //For status
  unsigned long lastTalkbackMillis = 0; //For talkback

  unsigned long currentMicros;
  unsigned long currentMillis;

  #define SEND_TIMEOUT                5   //When sending retries reaches this, the loop skips
  uint8_t Send_index = 0;

  #define WIFI_DISCONNECTION_TIMEOUT  10   //When reconnecting retries reaches this, the code completely renews the connection
  uint8_t Reconnecting_index = 0;

  #define THINGSPEAK_INTERVAL             (16 * 1000) //in seconds. the interval on status update. 1,000 is seconds to milliseconds conversion

//-------------------------------------LED-------------------------
  #define PM_LED_SENT_OK              13  //Green
  #define PM_LED_SENT_ERR             14  //Red
  #define RRH_LED_DISCONNECTED        16  //Blue

  #define STATUS_LED_SENT_OK          17  //Green
  #define STATUS_LED_SENT_ERR         18  //Red
  #define ESP32_LED_STATUS            27  //Blue

  #define GYRO_LED_DISCONNECTED       19  //Blue
  #define WiFi_LED_DISCONNECTED       23  //Red
  #define FAN_LED_WORKING             25  //Green

//-------------------------------------FAN-------------------------
  #define FAN_PIN                     26
  #define TEMPERATURE_THRESHOLD       29  //in °C

void setup() {
  Serial.begin(115200); 
  Serial.println("Starting.......");
  systemStatus |= 1<<4; //Forces bit 4 to 1. makes sure that thingspeak is notified whenever esp32 restarts
 //RRH62000  
  initializeRRH62000();
 //Gyro
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);  
  // Initialize MPU6500
  int err = IMU.init(calib, IMU_ADDRESS);
  while (err != 0) {
    digitalWrite(GYRO_LED_DISCONNECTED, HIGH);
    Serial.print("Error initializing MPU6500. Code: ");
    Serial.println(err);
    err = IMU.init(calib, IMU_ADDRESS);
    delay(1000);
  }
  
  // Set measurement range limits
  IMU.setAccelRange(8);   // Options: 2, 4, 8, 16 (g)
  IMU.setGyroRange(500);  // Options: 250, 500, 1000, 2000 (deg/s)
  
  digitalWrite(GYRO_LED_DISCONNECTED, LOW);
  Serial.println("IMU Working");
 //WiFi
  WiFi.mode(WIFI_STA); 
  
  Serial.print("Attempting to connect.....");
  while(WiFi.status() != WL_CONNECTED){
    digitalWrite(WiFi_LED_DISCONNECTED, HIGH);
    WiFi.begin(SSID, PASSWORD); 
    delay(5000);     
  } 
  digitalWrite(WiFi_LED_DISCONNECTED, LOW);
  Serial.println("\nConnected.");
  
 //Thingspeak
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  delay(1000);
  Serial.println("ThingSpeak Working");

 //LED
  pinMode(PM_LED_SENT_OK, OUTPUT);
  pinMode(PM_LED_SENT_ERR, OUTPUT);
  pinMode(RRH_LED_DISCONNECTED, OUTPUT);
  pinMode(STATUS_LED_SENT_OK, OUTPUT);
  pinMode(STATUS_LED_SENT_ERR, OUTPUT);
  pinMode(ESP32_LED_STATUS, OUTPUT);
  pinMode(GYRO_LED_DISCONNECTED, OUTPUT);
  pinMode(WiFi_LED_DISCONNECTED, OUTPUT);
  pinMode(FAN_LED_WORKING, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  Serial.println("LED Configured");

  digitalWrite(ESP32_LED_STATUS, HIGH);

}
void loop() {
  currentMicros = micros();
  currentMillis = millis();
 //Ensure WiFi is connected
  Reconnecting_index = 0;
  while(WiFi.status() != WL_CONNECTED){
    digitalWrite(WiFi_LED_DISCONNECTED, HIGH);
    Serial.println("Reconnecting..");
    WiFi.reconnect();
    delay(5000);
    Reconnecting_index ++;
   
    if(Reconnecting_index >= WIFI_DISCONNECTION_TIMEOUT){
      Reconnecting_index = 0;
      WiFi.disconnect();
      delay(2000);
      wifiConnect();
    }     
  } 
  digitalWrite(WiFi_LED_DISCONNECTED, LOW);
 //Gyro
  if(currentMicros - lastSampleMicros >= GYRO_SAMPLING_TIME ){
    lastSampleMicros = currentMicros;
    IMU.update();
    IMU.getAccel(&accelData);
    IMU.getGyro(&gyroData);
    
    //Calculate Roll and Pitch from Accelerometer
    float accelRoll = atan2(accelData.accelY, accelData.accelZ) * 180.0 / M_PI;
    float accelPitch = atan2(-accelData.accelX, sqrt(accelData.accelY * accelData.accelY + accelData.accelZ * accelData.accelZ)) * 180.0 / M_PI;
    //Complementary Filter. 96% Gyro integration + 4% Accelerometer anchor
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
  if(currentMillis - lastSampleMillis >= RRH_SAMPLING_TIME  ){
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

      enclosureFAN(RRH_sensor.temperature); //Condition for enclosure fan

      uploadStatus = ThingSpeak.writeFields(PM_CHANNEL_ID, PM_WRITE_API_KEY);
      while (uploadStatus != 200){
        Send_index ++;

        digitalWrite(PM_LED_SENT_ERR, HIGH);
        Serial.print("Problem updating PM channel. HTTP error code ");
        Serial.println(uploadStatus);
        
        WiFi.reconnect();
        delay(3000);
        uploadStatus = ThingSpeak.writeFields(PM_CHANNEL_ID, PM_WRITE_API_KEY);
      
        if(Send_index >= SEND_TIMEOUT){//Limit the retries
          break;
        }
      }
      digitalWrite(PM_LED_SENT_ERR, LOW);
      delay(50);
      digitalWrite(PM_LED_SENT_OK, HIGH);
      delay(300);
      digitalWrite(PM_LED_SENT_OK, LOW);

      if(Send_index >= SEND_TIMEOUT){
        Send_index = 0;
        Serial.println("PM Channel update Skipped");
      } else  Serial.println("PM Channel update successful."); 
      
      client.stop();

      if (RRH_sensor.status_fan_malfunction) {
        systemStatus |= 1 << 1; //Forces bit 1 to 1
      } else  systemStatus &= ~(1 << 1);  //Forces bit 1 to 0

      if (RRH_sensor.status_high_concentration){
        systemStatus |= 1 << 2; //Forces bit 2 to 1
      } else  systemStatus &= ~(1 << 2);  //Forces bit 2 to 0

      if (RRH_sensor.status_dust_accumulation) RRH_sensor.triggerManualCleaning();
    }
  }
 //Status
  if(systemStatus){ // != 0, wwhich means it has an error
    if(currentMillis - lastStatusMillis >= THINGSPEAK_INTERVAL ){ 
      lastStatusMillis = currentMillis;

      ThingSpeak.setField(1, systemStatus); 
      uploadStatus = ThingSpeak.writeFields(STATUS_CHANNEL_ID, STATUS_WRITE_API_KEY);

      while ( uploadStatus != 200){
        Send_index ++;
        
        digitalWrite(STATUS_LED_SENT_ERR, HIGH);
        Serial.println("Problem updating Status channel: Field 1. HTTP error code " + String(uploadStatus));
        WiFi.reconnect();
        delay(3000);
        uploadStatus = ThingSpeak.writeFields(STATUS_CHANNEL_ID, STATUS_WRITE_API_KEY);

        if(Send_index >= SEND_TIMEOUT){
          break;
        }
      }
      digitalWrite(STATUS_LED_SENT_ERR, LOW);
      delay(50);
      digitalWrite(STATUS_LED_SENT_OK, HIGH);
      delay(300);
      digitalWrite(STATUS_LED_SENT_OK, LOW);
      if(Send_index >= SEND_TIMEOUT){
        Send_index = 0;
        Serial.println("Status Channel: Field 1 update Skipped");  
      } else  Serial.println("Status Channel: Field 1 update successful.");
      
      client.stop();
      systemStatus = 0;
    }
  }
 //Talkback
  if(currentMillis - lastTalkbackMillis >= THINGSPEAK_INTERVAL){
    lastTalkbackMillis = currentMillis;
    checkTalkBackCommands();
  }
}

void wifiConnect(){
  Serial.print("Attempting to connect.....");
  while(WiFi.status() != WL_CONNECTED){
    digitalWrite(WiFi_LED_DISCONNECTED, HIGH);
    WiFi.begin(SSID, PASSWORD); 
    delay(5000);     
    Reconnecting_index ++;

    if(Reconnecting_index >= WIFI_DISCONNECTION_TIMEOUT){//Cant connect
      ESP.restart();  //restart ESP32
    }
  } 
  digitalWrite(WiFi_LED_DISCONNECTED, LOW);
  Serial.println("\nConnected.");

}

void enclosureFAN(float temperature){
  if(temperature >= TEMPERATURE_THRESHOLD){
    digitalWrite(FAN_PIN, HIGH);
    digitalWrite(FAN_LED_WORKING, HIGH);
    Serial.println("Fan Working");
  } else {
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(FAN_LED_WORKING, LOW);
    Serial.println("Fan OFF");
  }

}

void initializeRRH62000(){
  RRH_sensor.resetModule();
  if (!RRH_sensor.begin(SDA_PIN, SCL_PIN)) {
    Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
    while (!RRH_sensor.begin(SDA_PIN, SCL_PIN)){
      digitalWrite(RRH_LED_DISCONNECTED, HIGH);
      Serial.println("Failed to detect RRH62000 sensor. Check wiring & SEL pin!");
      delay(1000);
    }
  }
  // Configure module parameters via I2C
  RRH_sensor.setMovingAverage(60);          // Sampling interval should be MovingAverage * 3 
  RRH_sensor.setCleaningInterval(60480);    // Set auto-cleaning interval (60480 * 30s = 21 days). set to 21 days to make sure cleaning doesnt interrupt PM scanning
  RRH_sensor.setCleaningTime(60);           // Run fan cleaning for 60 seconds
  RRH_sensor.setFanSpeed(70);               // Set fan speed to 70%
  digitalWrite(RRH_LED_DISCONNECTED, LOW);
  Serial.println("RRH62000 Working");
}

void checkTalkBackCommands() {
  HTTPClient http;

  http.begin(TALKBACK_URL);
  int httpCode = http.POST(""); // POST request fetches and pops the command

  if (httpCode == HTTP_CODE_OK) {
    String command = http.getString();
    command.trim(); // Clean whitespace or newlines
      
    if (command.length() > 0) {
     //Restart ESP  
      if (command == "RESTART_ESP") {
        ESP.restart(); // Software reset the ESP32
      }
     //Restart RRH62000
      else if(command == "RESTART_RRH62000"){
        RRH_sensor.resetModule();
        initializeRRH62000();
        lastSampleMillis = millis(); //makes sure that there is a 3mins gap before sampling
      }
     //Set Fan Speed
      else if (command.startsWith("FAN_")) {
        String valueStr = command.substring(4); // Extract substring starting right after "FAN_" (Index 4 to end)
        int fanSpeed = valueStr.toInt(); // Converts string to integer
        RRH_sensor.setFanSpeed(fanSpeed);
        Serial.printf("Fan speed updated to %d%%\n", fanSpeed);
      }
     //Manual Cleaning
      else if(command = "Clean"){
        lastSampleMillis = millis();
        cleanRRH62000(true);
      }
    }
  } else {Serial.printf("TalkBack check failed, HTTP Code: %d\n", httpCode);
    http.end();
  }
}

void cleanRRH62000(bool cleaning){
  RRH_sensor.triggerManualCleaning();


}

