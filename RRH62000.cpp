#include "RRH62000.h"

RRH62000::RRH62000(uint8_t address) {
    _address = address;
    _wire = &Wire;
    
    // Clear initial variable values
    pm1_0_kcl = pm2_5_kcl = pm10_0_kcl = 0.0f;
    pm1_0_smoke = pm2_5_smoke = pm10_0_smoke = 0.0f;
    temperature = humidity = iaq = 0.0f;
    tvoc = eco2 = 0;
}

bool RRH62000::begin(int sdaPin, int sclPin, uint32_t frequency, TwoWire &wirePort) {
    _wire = &wirePort;
    
    // Custom ESP32 pin configuration for TwoWire
    if (sdaPin != -1 && sclPin != -1) {
        if (!_wire->begin(sdaPin, sclPin, frequency)) {
            return false;
        }
    } else {
        if (!_wire->begin()) {
            return false;
        }
        _wire->setClock(frequency);
    }

    // Verify presence of sensor on I2C bus
    _wire->beginTransmission(_address);
    return (_wire->endTransmission() == 0);
}

bool RRH62000::isDataReady() {
    _wire->beginTransmission(_address);
    _wire->write(0x40); // Command: DATA availability check
    if (_wire->endTransmission() != 0) return false;

    if (_wire->requestFrom(_address, (uint8_t)1) == 1) {
        return (_wire->read() == 0x01); // 0x01 = New data arrived
    }
    return false;
}

bool RRH62000::readSensor() {
    // Send READ command (0x00)
    _wire->beginTransmission(_address);
    _wire->write(0x00);
    if (_wire->endTransmission() != 0) {
        return false;
    }

    // Request 37 measurement bytes from sensor
    uint8_t buffer[37];
    uint8_t bytesReceived = _wire->requestFrom(_address, (uint8_t)37);
    if (bytesReceived != 37) {
        return false;
    }

    for (int i = 0; i < 37; i++) {
        buffer[i] = _wire->read();
    }

    // Validate CRC8 over bytes 0 to 35 against byte 36
    uint8_t calculatedCRC = calculateCRC8(buffer, 36);
    if (calculatedCRC != buffer[36]) {
        return false; // Checksum failed
    }

    // Parse Big-Endian values & apply scaling factors
    pm1_0_kcl    = ((uint16_t)(buffer[12] << 8 | buffer[13])) * 0.1f;
    pm2_5_kcl    = ((uint16_t)(buffer[14] << 8 | buffer[15])) * 0.1f;
    pm10_0_kcl   = ((uint16_t)(buffer[16] << 8 | buffer[17])) * 0.1f;

    pm1_0_smoke  = ((uint16_t)(buffer[18] << 8 | buffer[19])) * 0.1f;
    pm2_5_smoke  = ((uint16_t)(buffer[20] << 8 | buffer[21])) * 0.1f;
    pm10_0_smoke = ((uint16_t)(buffer[22] << 8 | buffer[23])) * 0.1f;

    temperature  = ((int16_t)(buffer[24] << 8 | buffer[25])) * 0.01f;
    humidity     = ((uint16_t)(buffer[26] << 8 | buffer[27])) * 0.01f;
    tvoc         = ((uint16_t)(buffer[28] << 8 | buffer[29])) * 10;
    eco2         = ((uint16_t)(buffer[30] << 8 | buffer[31]));
    iaq          = ((uint16_t)(buffer[32] << 8 | buffer[33])) * 0.01f;

    return true;
}

// CRC-8 calculation (Polynomial: 0x31, Init: 0xFF, No reflect, No final XOR)
uint8_t RRH62000::calculateCRC8(const uint8_t *data, size_t length) {
    uint8_t crc = 0xFF; // Initialization 0xFF
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31; // Polynomial 0x31
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}