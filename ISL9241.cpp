#include "ISL9241.h"    

ISL9241::ISL9241(uint8_t read_address = ISL9241_ADDRESS) {
	_smb_address = read_address;
}

bool ISL9241::init(unsigned int NoOfCells = 2, float minBattVoltage = 2.8, float maxBattVoltage = 3.2) {
  Wire.begin();

	if (NoOfCells < 2 || NoOfCells > 4) {
		// strcpy(ISL9241::error_s,"ISL9241 supports 2,3 and 4 batteries in series\n");
		// error_t = Error_t::INVALID_PARAMETER;
		return false;
	}

	if(NoOfCells*minBattVoltage > SYSTEM_MAX_VOLTAGE || NoOfCells*maxBattVoltage > SYSTEM_MAX_VOLTAGE) {
    // strcpy(error_s,"Battery configuration exceeds maximum system voltage\n");
		// error_t = Error_t::INVALID_PARAMETER;
		return false;
	}

	// Check if the device is connected
	uint16_t value;
	if(!readRegister(DeviceID, &value) || value != 0x000E) {
    // strcpy(error_s,"Device not found\n");
		// error_t = Error_t::DEVICE_NOT_FOUND;
		return false;
	}
	
	if(setMaxSystemVoltage(NoOfCells*maxBattVoltage) == -1 || setMinSystemVoltage(NoOfCells*minBattVoltage) == -1) /*return false;*/

	// enable adc for all modes
	return writeBit(Control3, 0, true);
}

/**
 * @brief Write a value to a register using the SMBus protocol
*/
bool ISL9241::writeRegister(uint16_t reg, uint16_t value) {
	Wire.beginTransmission(_smb_address);
	Wire.write(reg);
	Wire.write(value & 0x00FF);
	Wire.write((value >> 8) & 0x00FF);
	
	if(Wire.endTransmission() != 0) {
		// strcpy(error_s, "Could not write to register\n");
		// error_t = Error_t::REGISTER_WRITE_ERROR;
		return false;
	}

	return true;
}

/**
 * @brief Read a value from a register using the SMBus protocol
*/
bool ISL9241::readRegister(uint16_t reg, uint16_t *value) {
	Wire.beginTransmission(_smb_address);
	Wire.write(reg);
	if (Wire.endTransmission() != 0) {
		// strcpy(error_s, "Could initiate read from register\n");
		// error_t = Error_t::REGISTER_READ_ERROR;
		return false;
	}
  

	Wire.requestFrom(_smb_address, 2);
	if (Wire.available() < 2) {
		// strcpy(error_s, "Data requested not available\n");
		// error_t = Error_t::REGISTER_READ_ERROR;
		return false;
	}

	uint8_t lowByte = Wire.read();
	uint8_t highByte = Wire.read();
  
	*value = (highByte << 8) | lowByte;

	if(Wire.endTransmission() != 0){
		// strcpy(error_s, "Transmission error\n");
		// error_t = Error_t::REGISTER_READ_ERROR;
		return false;
	}

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

bool ISL9241::readBit(uint16_t reg, uint8_t bit) {
	uint16_t regValue;
	if (!readRegister(reg, &regValue)) {
		return false;
	}
	return (regValue & (1 << bit)) != 0;
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
	if(readRegister(VSYS_ADC_Result, &value)) return -1;

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	value = (value & mask) >> 6;

	return value * SYS_VOLTAGE_ADC_LSB;
}

float ISL9241::getAdapterCurrent() {
	uint16_t value;
	if (!readRegister(IADP_ADC_Results, &value)) return -1;

  	int8_t mask = 0xff;

	return ((value & mask) * IADP_ADC_LSB);
}

float ISL9241::getAdapterVoltage() {
	uint16_t value;

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	if (!readRegister(VIN_ADC_Result, &value)) return -1;
	value = (value & mask) >> 6;
	return value * VIN_ADC_LSB;
}

float ISL9241::getBattDischargeCurrent() {
	uint16_t value;
	if (!readRegister(DC_ADC_Results, &value)) return -1;

	// mask for bits 7:0

	return value * BATT_DISCHARGE_CURRENT_ADC_LSB;
}

float ISL9241::getBattChargeCurrent() {
	uint16_t value;
	if(!readRegister(CC_ADC_Result, &value)) return -1;

	// mask for bits 7:0

	return value * BATT_CHARGE_CURRENT_ADC_LSB;
}

float ISL9241::getBatteryVoltage() {
	uint16_t value;
	if(!readRegister(VBAT_ADC_Results, &value)) return -1;

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	value = (value & mask) >> 6;
	return value * V_BAT_LSB;
}

float ISL9241::setAdapterCurrentLimit(float current) {
  if (current < 0 ) return false;

	uint16_t reg = (uint16_t)(current / ADAPTER_CURRENT_LIMIT_LSB_VAL);
  	if(reg > (2 << 10)) reg = 2 << 10;

	if(!writeRegister(AdapterCurrentLimit1, reg << 2))
		return -1;

	return reg*ADAPTER_CURRENT_LIMIT_LSB_VAL;
}

float ISL9241::getAdapterCurrentLimit() {
	uint16_t value;
	if(!readRegister(AdapterCurrentLimit1, &value)) return -1;

  	// mask for bits 12:2
	uint16_t mask = 0x1FFC;

	value = (value & mask) >> 2;

	return value * ADAPTER_CURRENT_LIMIT_LSB_VAL;
}

float ISL9241::setChargeCurrentLimit(float current) {
	uint16_t reg = (uint16_t)(current / ADAPTER_CURRENT_LIMIT_LSB_VAL) << 2;

	if(!writeRegister(ChargeCurrentLimit, reg))
		return -1;

	return (reg >> 2) * ADAPTER_CURRENT_LIMIT_LSB_VAL;
}

float ISL9241::getChargeCurrentLimit() {
	uint16_t value;
	if(!readRegister(ChargeCurrentLimit, &value)) return -1;

	// mask for bits 12:2
	uint16_t mask = 0x1FFC;

	value = (value & mask) >> 2;

	return value * 4e-3;
}

float ISL9241::setMaxSystemVoltage(float voltage) {
	uint16_t voltage_reg = (uint16_t)(voltage / 8e-3) << 3;
	
	if(!writeRegister(MaxSystemVoltage, voltage_reg))
		return -1;

	return (voltage_reg >> 3) * 8e-3;
}

float ISL9241::getMaxSystemVoltage() {
	uint16_t value;
	if(!readRegister(MaxSystemVoltage, &value)) return -1;

	// mask for bits 14:3
	uint16_t mask = 0x3FF8;
	value = (value & mask) >> 3;
	return value * 8e-3;
}

float ISL9241::setMinSystemVoltage(float voltage) {
	uint16_t reg = (uint16_t)(voltage / 64e-3) << 6;

	if(!writeRegister(MinSystemVOltage, reg))
		return -1;

	return (reg >> 6) * 64e-3;
}

float ISL9241::getMinSystemVoltage() {
	uint16_t value;
	if(!readRegister(MinSystemVOltage, &value)) return -1;

	// mask for bits 13:6
	uint16_t mask = 0x3FC0;

	value = (value & mask) >> 6;
	return value * 64e-3;
}

float ISL9241::setAdapterCurrentLimit2(float current) {
	uint16_t reg = (uint16_t)(current / 4e-3) << 3;
	if(!writeRegister(AdapterCurrentLimit2, reg))
		return -1;

	return (reg >> 3) * 4e-3;
}

float ISL9241::getAdapterCurrentLimit2() {
	uint16_t value;
	if(!readRegister(AdapterCurrentLimit2, &value)) return -1;

	// mask for bits 12:2
	uint16_t mask = 0x1FFC;

	value = (value & mask) >> 2;
	return value * 4e-3;
}

float ISL9241::setTricleChargeCurrent(TCCL_t lim) {
	uint16_t reg = (uint16_t)lim << 2;
	if(!writeRegister(ChargeCurrentLimit, reg))
		return -1;

	return (reg >> 2) * 4e-3;
}

/**
 * @brief Enable or disable the NGATE which cuts off the battery from the system.
 * 
 * @param value Set to true to disconect the battery from the system.
 * @return true if the operation was successful. 
 * @return false if the operation failed. 
 */
bool ISL9241::setNGATE(bool value) {
	return writeBit(Control1, 12, value);
}

/**
 * @brief Indicates the ISL9241 state machine status.
 * 
 * @note The state machine status is bits 8-11 from register Information2 at 0x4D.
 * @relates ISL9241::Information2
 * @return uint8_t 
 */
StateMachineStatus_t ISL9241::getStateMachineStatus() {
	uint16_t value;
	if(!readRegister(Information2, &value)) return RESET;

	// mask for bits 11:8
	uint16_t mask = 0x0F00;

	value = (value & mask) >> 8;

	return (StateMachineStatus_t)value;
}


