#include "RRH62000.h"

RRH62000::RRH62000(uint8_t address) {
    _address = address;
    _wire = &Wire;
    
    // Clear initial variable values
    status = relative_iaq = 0;
    status_high_concentration = status_dust_accumulation = false;
    status_fan_speed_error = status_fan_malfunction = false;

    nc_0_3 = nc_0_5 = nc_1_0 = nc_2_5 = nc_4_0 = 0.0f;
    pm1_0_kcl = pm2_5_kcl = pm10_0_kcl = 0.0f;
    pm1_0_smoke = pm2_5_smoke = pm10_0_smoke = 0.0f;
    temperature = humidity = iaq = 0.0f;
    tvoc = eco2 = 0;

    mox_resistance = 0;
    tvoc_cleaning_done = false;
    memset(unique_id, 0, sizeof(unique_id));
    memset(algo_version, 0, sizeof(algo_version));
    memset(firmware_version, 0, sizeof(firmware_version));
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

    _wire->readBytes(buffer, 37);

    // Validate CRC8 over bytes 0 to 35 against byte 36
    uint8_t calculatedCRC = calculateCRC8(buffer, 36);
    if (calculatedCRC != buffer[36]) {
        return false; // Checksum failed
    }

    // --- Bytes 0-1: Status ---
    status = (uint16_t)(buffer[0] << 8 | buffer[1]);
    status_high_concentration = (buffer[1] & 0x01);
    status_dust_accumulation  = (buffer[1] >> 1) & 0x01;
    status_fan_speed_error    = (buffer[1] >> 2) & 0x01;
    status_fan_malfunction    = (buffer[1] >> 3) & 0x01;

    // --- Bytes 2-11: Number Concentrations (0.1 /cm³) ---
    nc_0_3 = ((uint16_t)(buffer[2] << 8 | buffer[3])) * 0.1f;
    nc_0_5 = ((uint16_t)(buffer[4] << 8 | buffer[5])) * 0.1f;
    nc_1_0 = ((uint16_t)(buffer[6] << 8 | buffer[7])) * 0.1f;
    nc_2_5 = ((uint16_t)(buffer[8] << 8 | buffer[9])) * 0.1f;
    nc_4_0 = ((uint16_t)(buffer[10] << 8 | buffer[11])) * 0.1f;

    // --- Bytes 12-23: PM Mass Concentrations (0.1 ug/m³) ---
    pm1_0_kcl    = ((uint16_t)(buffer[12] << 8 | buffer[13])) * 0.1f;
    pm2_5_kcl    = ((uint16_t)(buffer[14] << 8 | buffer[15])) * 0.1f;
    pm10_0_kcl   = ((uint16_t)(buffer[16] << 8 | buffer[17])) * 0.1f;

    pm1_0_smoke  = ((uint16_t)(buffer[18] << 8 | buffer[19])) * 0.1f;
    pm2_5_smoke  = ((uint16_t)(buffer[20] << 8 | buffer[21])) * 0.1f;
    pm10_0_smoke = ((uint16_t)(buffer[22] << 8 | buffer[23])) * 0.1f;

    // --- Bytes 24-35: Environment & Gas Sensors ---
    temperature  = ((int16_t)(buffer[24] << 8 | buffer[25])) * 0.01f;
    humidity     = ((uint16_t)(buffer[26] << 8 | buffer[27])) * 0.01f;
    tvoc         = ((uint16_t)(buffer[28] << 8 | buffer[29])) * 10;
    eco2         = ((uint16_t)(buffer[30] << 8 | buffer[31]));
    iaq          = ((uint16_t)(buffer[32] << 8 | buffer[33])) * 0.01f;
    relative_iaq = ((uint16_t)(buffer[34] << 8 | buffer[35]));

    return true;
}

// Command 0x50: Put module into sleep mode
bool RRH62000::sleep() {
    return writeRegister(0x50, 0x00);
}

// Command 0x50: Wake module up
bool RRH62000::wakeUp() {
    return writeRegister(0x50, 0x80);
}

// Command 0x51: Manually start fan dust-cleaning process
bool RRH62000::triggerManualCleaning() {
    return writeRegister(0x51, 0x01);
}

// Command 0x52: Trigger a software reset (same as power-on reset)
bool RRH62000::resetModule() {
    return writeRegister(0x52, 0x81);
}

// Set Moving Average filter length (1 to 60 samples)
bool RRH62000::setMovingAverage(uint8_t samples) {
    if (samples < 1 || samples > 60) return false;
    return writeRegister(0x53, samples); // Reg 0x53 MAVE
}

// Set Auto-Cleaning Interval (0 to 60480 in 30-second increments)
bool RRH62000::setCleaningInterval(uint16_t interval30s) {
    if (interval30s > 60480) return false;
    
    uint8_t highByte = (interval30s >> 8) & 0xFF;
    uint8_t lowByte  = interval30s & 0xFF;

    if (!writeRegister(0x5A, highByte)) return false; // Reg 0x5A TINTC_H
    return writeRegister(0x5B, lowByte);              // Reg 0x5B TINTC_L
}

// Set Fan Auto-Cleaning Duration (0 to 60 seconds)
bool RRH62000::setCleaningTime(uint8_t seconds) {
    if (seconds > 60) return false;
    return writeRegister(0x5C, seconds); // Reg 0x5C TCLEAN
}

// Set Fan Speed percentage (60% to 100%)
bool RRH62000::setFanSpeed(uint8_t speedPercent) {
    if (speedPercent < 60 || speedPercent > 100) return false;
    return writeRegister(0x63, speedPercent); // Reg 0x63 SPEEDFAN
}

// Command 0x71: Read MOX[6] resistance (4 Bytes)
bool RRH62000::readMoxResistance() {
    uint8_t buf[4];
    if (!readRegisterBytes(0x71, buf, 4)) return false;
    mox_resistance = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    return true;
}

// Command 0x72: Read Unique Hardware ID (6 Bytes)
bool RRH62000::readUniqueID() {
    return readRegisterBytes(0x72, unique_id, 6);
}

// Command 0x73: Read Algorithm Version (3 Bytes: Major, Minor, Patch)
bool RRH62000::readAlgorithmVersion() {
    return readRegisterBytes(0x73, algo_version, 3);
}

// Command 0x74: Read TVOC sensor cleaning status (1 Byte)
bool RRH62000::readCleaningStatus() {
    uint8_t statusByte = 0;
    if (!readRegisterBytes(0x74, &statusByte, 1)) return false;
    tvoc_cleaning_done = (statusByte == 0x01);
    return true;
}

// Command 0x75: Read Firmware Version (2 Bytes: Major, Minor)
bool RRH62000::readFirmwareVersion() {
    return readRegisterBytes(0x75, firmware_version, 2);
}

// --- Helper Register Writer ---
bool RRH62000::writeRegister(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

bool RRH62000::readRegisterBytes(uint8_t reg, uint8_t *buffer, size_t length) {
    _wire->beginTransmission(_address);
    _wire->write(reg);
    if (_wire->endTransmission() != 0) return false;

    if (_wire->requestFrom(_address, (uint8_t)length) != length) return false;

    for (size_t i = 0; i < length; i++) {
        buffer[i] = _wire->read();
    }
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