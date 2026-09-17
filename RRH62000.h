#ifndef RRH62000_H
#define RRH62000_H

#include <Arduino.h>
#include <Wire.h>

// Default I2C Address for RRH62000 (SEL pin pulled LOW)
#define RRH62000_DEFAULT_I2C_ADDR 0x69 

class RRH62000 {
public:
    // Stored sensor measurement variables
    float pm1_0_kcl;      // ug/m3 (KCl reference)
    float pm2_5_kcl;      // ug/m3 (KCl reference)
    float pm10_0_kcl;     // ug/m3 (KCl reference)
    float pm1_0_smoke;    // ug/m3 (Cigarette smoke reference)
    float pm2_5_smoke;    // ug/m3 (Cigarette smoke reference)
    float pm10_0_smoke;   // ug/m3 (Cigarette smoke reference)
    float temperature;    // °C
    float humidity;       // % RH
    uint16_t tvoc;        // ug/m3
    uint16_t eco2;        // ppm
    float iaq;            // Indoor Air Quality Index (UBA standard)

    RRH62000(uint8_t address = RRH62000_DEFAULT_I2C_ADDR);

    // Initialize I2C with custom ESP32 SDA and SCL pins
    bool begin(int sdaPin = -1, int sclPin = -1, uint32_t frequency = 100000, TwoWire &wirePort = Wire);

    // Check if new data is available to read
    bool isDataReady();

    // Read full 37-byte measurement packet, validate CRC, and update internal variables
    bool readSensor();

private:
    uint8_t _address;
    TwoWire *_wire;

    uint8_t calculateCRC8(const uint8_t *data, size_t length);
};

#endif // RRH62000_H