#include "ISL9241.h"    
#include <Arduino.h>

ISL9241::ISL9241(uint8_t read_address = ISL9241_ADDRESS) {
	_smb_address = read_address;
}

bool ISL9241::init(unsigned int NoOfCells = 2) {
  Wire.begin();

	// Check if the device is connected
	uint16_t value;
	readRegister(DeviceID, &value);
	if (value != 0x9241) {
		return false;
	}

	writeRegister(AdapterCurrentLimit1, 0x01D4);		// 0.468A
	writeRegister(ChargeCurrentLimit	, 0x07D0);		// 2.000 A

	switch (NoOfCells)
	{
	case 2:
		writeRegister(MaxSystemVoltage		, 0x20C0);		// 8.384 V
		writeRegister(MinSystemVOltage		, 0x5400);		// 4.376 V
		break;
	case 3:
		writeRegister(MaxSystemVoltage		, 0x3120);		// 12.576 V
		writeRegister(MinSystemVOltage		, 0x1C00);		// 07.168 V
		break;
	case 4:
		writeRegister(MaxSystemVoltage		, 0x4180);		// 16.768 V
		writeRegister(MinSystemVOltage		, 0x2700);		// 10.000 V
		break;
	
	default:
		return false;
		break;
	}

	// enable adc for all modes
	writeBit(Control3, 0, true);
	

  return true;
}

/**
 * @brief Write a value to a register using the SMBus protocol
*/
bool ISL9241::writeRegister(uint16_t reg, uint16_t value) {
	Wire.beginTransmission(_smb_address);
	Wire.write(reg);
	Wire.write(value & 0x00FF);
	Wire.write((value >> 8) & 0x00FF);
	return Wire.endTransmission() == 0;
}

/**
 * @brief Read a value from a register using the SMBus protocol
*/
bool ISL9241::readRegister(uint16_t reg, uint16_t *value) {
	Wire.beginTransmission(_smb_address);
	Wire.write(reg);
	if (Wire.endTransmission() != 0) {
    Serial.println("Nope");
		return false;
	}
  

	Wire.requestFrom(_smb_address, 2);
	if (Wire.available() < 2) {
    Serial.println("Balls");
		return false;
	}

	uint8_t lowByte = Wire.read();
	uint8_t highByte = Wire.read();

  // Serial.print("Low byte : ");
	// Serial.println(lowByte);
	// Serial.print("High byte : ");
	// Serial.println(highByte);
  
	*value = (highByte << 8) | lowByte;

	return true;
}

bool ISL9241::writeBit(uint16_t reg, uint8_t bit, bool value) {
	uint16_t regValue;
	if (!readRegister(reg, &regValue)) {
		return false;
	}
	if (value) {
		regValue |= (1 << bit);
	} else {
		regValue &= ~(1 << bit);
	}
	return writeRegister(reg, regValue);
}

bool ISL9241::readBit(uint16_t reg, uint8_t bit, bool *value) {
	uint16_t regValue;
	if (!readRegister(reg, &regValue)) {
		return false;
	}
	*value = (regValue & (1 << bit)) != 0;
	return true;
}

float ISL9241::SetSysVoltage(float voltage) {
	uint16_t value = (uint16_t)(voltage / SYS_VOLTAGE_ADC_LSB);

	float retVal = value * SYS_VOLTAGE_ADC_LSB;

	// shift the value 6 bits to the left
	value = value << 6;

	writeRegister(MaxSystemVoltage, value);

	return retVal;
}

float ISL9241::GetSysVoltage() {
	uint16_t value;
	readRegister(MaxSystemVoltage, &value);

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	value = (value & mask) >> 6;

	return value * SYS_VOLTAGE_ADC_LSB;
}

float ISL9241::getAdapterCurrent() {
	uint16_t value;
	readRegister(IADP_ADC_Results, &value);

  int8_t mask = 0xff;

	return ((value & mask) * IADP_ADC_LSB);
}

float ISL9241::getAdapterVoltage() {
	uint16_t value;

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	readRegister(VIN_ADC_Result, &value);
	value = (value & mask) >> 6;
	return value * VIN_ADC_LSB;
}

float ISL9241::getBattDischargeCurrent() {
	uint16_t value;
	readRegister(DC_ADC_Results, &value);

	// mask for bits 7:0

	return value * BATT_DISCHARGE_CURRENT_ADC_LSB;
}

float ISL9241::getBattChargeCurrent() {
	uint16_t value;
	readRegister(CC_ADC_Result, &value);

	// mask for bits 7:0

	return value * BATT_CHARGE_CURRENT_ADC_LSB;
}

float ISL9241::getBatteryVoltage() {
	uint16_t value;
	readRegister(VBAT_ADC_Results, &value);

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	value = (value & mask) >> 6;
	return value * V_BAT_LSB;
}

float ISL9241::setAdapterCurrentLimit(float current) {
  if (current < 0 ) return false;

	uint16_t reg = (uint16_t)(current / ADAPTER_CURRENT_LIMIT_LSB_VAL);
  if(reg > (2 << 10)) reg = 2 << 10;

	writeRegister(AdapterCurrentLimit1, reg << 2);

	return reg*ADAPTER_CURRENT_LIMIT_LSB_VAL;
}

float ISL9241::getAdapterCurrentLimit() {
	uint16_t value;
	readRegister(AdapterCurrentLimit1, &value);

  // mask for bits 12:2
	uint16_t mask = 0x1FFC;

	value = (value & mask) >> 2;

	return value * ADAPTER_CURRENT_LIMIT_LSB_VAL;
}

float ISL9241::setChargeCurrentLimit(float current) {
	uint16_t reg = (uint16_t)(current / ADAPTER_CURRENT_LIMIT_LSB_VAL) << 2;

	writeRegister(ChargeCurrentLimit, reg);

	return (reg >> 2) * ADAPTER_CURRENT_LIMIT_LSB_VAL;
}

float ISL9241::getChargeCurrentLimit() {
	uint16_t value;
	readRegister(ChargeCurrentLimit, &value);

	// mask for bits 12:2
	uint16_t mask = 0x1FFC;

	value = (value & mask) >> 2;

	return value * 4e-3;
}

float ISL9241::setMaxSystemVoltage(float voltage) {
	uint16_t voltage_reg = (uint16_t)(voltage / 8e-3) << 3;
	
	writeRegister(MaxSystemVoltage, voltage_reg);

	return (voltage_reg >> 3) * 8e-3;
}

float ISL9241::getMaxSystemVoltage() {
	uint16_t value;
	readRegister(MaxSystemVoltage, &value);

	// mask for bits 14:3
	uint16_t mask = 0x3FF8;
	value = (value & mask) >> 3;
	return value * 8e-3;
}

float ISL9241::setMinSystemVoltage(float voltage) {
	uint16_t reg = (uint16_t)(voltage / 64e-3) << 6;

	writeRegister(MinSystemVOltage, reg);

	return (reg >> 6) * 64e-3;
}

float ISL9241::getMinSystemVoltage() {
	uint16_t value;
	readRegister(MinSystemVOltage, &value);

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	value = (value & mask) >> 6;
	return value * 64e-3;
}

float ISL9241::setAdapterCurrentLimit2(float current) {
	uint16_t reg = (uint16_t)(current / 4e-3) << 3;
	writeRegister(AdapterCurrentLimit2, reg);

	return (reg >> 3) * 4e-3;
}

float ISL9241::getAdapterCurrentLimit2() {
	uint16_t value;
	readRegister(AdapterCurrentLimit2, &value);

	// mask for bits 12:2
	uint16_t mask = 0x1FFC;

	value = (value & mask) >> 2;
	return value * 4e-3;
}

float ISL9241::setTricleChargeCurrent(TCCL_t lim) {
	uint16_t reg = (uint16_t)lim << 2;
	// writeRegister(TrickleChargeCurrentLimit, reg);

	return (reg >> 2) * 4e-3;
}




