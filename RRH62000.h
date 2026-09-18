#ifndef RRH62000_H
#define RRH62000_H

#include <Arduino.h>
#include <Wire.h>

// Default I2C Address for RRH62000 (SEL pin pulled LOW)
#define RRH62000_DEFAULT_I2C_ADDR 0x69 

class RRH62000 {
public:
    // --- Status Flags (Bytes 0-1) ---
    uint16_t status;                   // Raw status register
    bool status_high_concentration;    // Bit 0: Concentration extremely high
    bool status_dust_accumulation;     // Bit 1: Dust accumulated inside module
    bool status_fan_speed_error;       // Bit 2: Fan speed out of set range
    bool status_fan_malfunction;       // Bit 3: Fan malfunctioned/broken

    // --- Number Concentrations in 0.1 /cm³ (Bytes 2-11) ---
    float nc_0_3;   // Particle size 0.3μm - 10μm
    float nc_0_5;   // Particle size 0.5μm - 10μm
    float nc_1_0;   // Particle size 1.0μm - 10μm
    float nc_2_5;   // Particle size 2.5μm - 10μm
    float nc_4_0;   // Particle size 4.0μm - 10μm

    // --- Mass Concentrations in ug/m³ (Bytes 12-23) ---
    float pm1_0_kcl;      // KCl reference
    float pm2_5_kcl;      // KCl reference
    float pm10_0_kcl;     // KCl reference
    float pm1_0_smoke;    // Cigarette smoke reference
    float pm2_5_smoke;    // Cigarette smoke reference
    float pm10_0_smoke;   // Cigarette smoke reference

    // --- Environmental & Air Quality Data (Bytes 24-35) ---
    float temperature;     // °C (Byte 24-25)
    float humidity;        // % RH (Byte 26-27)
    uint16_t tvoc;         // ug/m³ (Byte 28-29)
    uint16_t eco2;         // ppm (Byte 30-31)
    float iaq;             // Indoor Air Quality Index (Byte 32-33)
    uint16_t relative_iaq; // Reserved output (Byte 34-35)

    RRH62000(uint8_t address = RRH62000_DEFAULT_I2C_ADDR);

    // Initialize I2C with custom ESP32 SDA and SCL pins
    bool begin(int sdaPin = -1, int sclPin = -1, uint32_t frequency = 100000, TwoWire &wirePort = Wire);

    bool isDataReady(); // Check if new data is available to read
    bool readSensor();  // Read full 37-byte measurement packet, validate CRC, and update internal variables
    

private:
    uint8_t _address;
    TwoWire *_wire;

    uint8_t calculateCRC8(const uint8_t *data, size_t length);
};

#endif // RRH62000_H