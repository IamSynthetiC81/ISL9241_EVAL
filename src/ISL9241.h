#ifndef ISL9241_H
#define ISL9241_H

#ifndef ARDUINO
	#error "This library is intended to be used with the Arduino IDE.\
			Either use  the Arduino IDE \
			or modify the code to work with your platform"
#endif

#include <stdint.h>
#include <Wire.h>
#include <string.h>

#define ISL9241_MIN_OPERATING_VOLTAGE 3.9
#ifndef ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD
	#define ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD 4.0
#elif ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD < ISL9241_MIN_OPERATING_VOLTAGE
	#error "ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD should be greater than ISL9241_MIN_OPERATING_VOLTAGE"
#endif

#ifndef ISL9241_BATT_NUM
	#define ISL9241_BATT_NUM 2
#elif ISL9241_BATT_NUM < 2
	#error "ISL9241 supports 2 or more batteries in series" 
#endif


// I2C Address for ISL9241
#define ISL9241_ADDRESS 0x09

// ISL9241 Register Addresses
#define ChargeCurrentLimit         0x14
#define MaxSystemVoltage           0x15
#define Control7                   0x3B
#define Control0                   0x39
#define Information1               0x3A
#define AdapterCurrentLimit2       0x3B
#define Control1                   0x3C
#define Control2                   0x3D
#define MinSystemVOltage           0x3E
#define AdapterCurrentLimit1       0x3F
#define ACOK_reference             0x40
#define Control6                   0x43
#define ACProchot                  0x47
#define DCProchot                  0x48
#define OTG_Voltage                0x49
#define OTG_Current                0x4A
#define VinVoltage                 0x4B
#define Control3                   0x4C
#define Information2               0x4D
#define Control4                   0x4E
#define Control5                   0x4F
#define NTC_ADC_Result             0x80
#define VBAT_ADC_Results           0x81
#define TJ_ADC_Results             0x82
#define IADP_ADC_Results           0x83
#define DC_ADC_Results             0x84
#define CC_ADC_Result              0x85
#define VSYS_ADC_Result            0x86
#define VIN_ADC_Result             0x87
#define Information3               0x90
#define Information4               0x91
#define ManufacturerID             0xFE
#define DeviceID                   0xFF


#define NTC_VOLTAGE_LSB 8e-3
#define V_BAT_LSB 64e-3
#define TJ_ADC_LSB 8e-3
#define IADP_ADC_LSB 22.2e-3
#define BATT_DISCHARGE_CURRENT_ADC_LSB 44.4e-3
#define BATT_CHARGE_CURRENT_ADC_LSB 22.2e-3
#define SYS_VOLTAGE_ADC_LSB 96e-3
#define VIN_ADC_LSB 96e-3

#define DAC_CHARGE_CURRENT_LIMIT_LSB 4e-3
#define DAC_SYSTEM_MAX_VOLTAGE_LSB 8e-3
#define DAC_SYSTEM_MIN_VOLTAGE_LSB 64e-3
#define DAC_ADAPTER_CURRENT_LIMIT_LSB 4e-3
#define DAC_ADAPTER_CURRENT_LIMIT2_LSB 4e-3
#define DAC_ADP_MIN_VOLTAGE_LSB 85e-3

#define ADAPTER_CURRENT_LIMIT_LSB_VAL 4e-3
#define SYSTEM_MAX_VOLTAGE 18.304

// enum Error_t{
// 	NO_ERROR = 0,
// 	DEVICE_NOT_FOUND = 1,
// 	REGISTER_WRITE_ERROR = 2,
// 	REGISTER_READ_ERROR = 3,
// 	REGISTER_BIT_WRITE_ERROR = 4,
// 	REGISTER_BIT_READ_ERROR = 5,
// 	INVALID_PARAMETER = 6
// };
// char error_s[64];
// Error_t error_t;

/**
* Tricle_Charge_Current_Limit
*/
enum TCCL_t {
	TC_32mA = 0,
	TC_64mA = 1,
	TC_128mA = 2,
	TC_160mA = 3,
	TC_192mA = 4,
	TC_224mA = 5,
	TC_256mA = 6,
};

/**
 * @brief Indicates the ISL9241 state machine status
 * 
 * @see Information2 at 0x4D page 39 out of 75.
 * @link https://www.farnell.com/datasheets/2710985.pdf
 */
enum StateMachineStatus_t{
	RESET = 0,
	INTERNAL_WAKE_UP_1 = 1,
	AUTO_CHARGE = 2,
	INTERNAL_WAKE_UP_2 = 3,
	BATT_ONLY = 4,
	INTERNAL_WAKE_UP_3 = 5, 
	SMB_CHARGE = 6,
	FAULT = 7,
	INTERNAL_WAKE_UP_4 = 8,
	OTG = 9,
	READY = 10,
	INTERNAL_WAKE_UP_5 = 11,
	VSYS = 12,
};

class ISL9241{
	public:
		ISL9241(uint8_t read_address = ISL9241_ADDRESS);
		bool init(unsigned int NoOfCells = 2, float minBattVoltage = 2.8, float maxBattVoltage = 3.2);
		
    	bool writeRegister(uint16_t reg, uint16_t value);
		bool readRegister(uint16_t reg, uint16_t *value);

   		bool writeBit(uint16_t reg, uint8_t bit, bool value);
		bool readBit(uint16_t reg, uint8_t bit, bool *value);

		float SetSysVoltage(float voltage);
		float GetSysVoltage();

		float getBattDischargeCurrent();
		float getBattChargeCurrent();

		float getAdapterVoltage();
		float getAdapterCurrent();

		float getBatteryVoltage();

		float setAdapterCurrentLimit(float current);
    	float getAdapterCurrentLimit();
    
		float setChargeCurrentLimit(float current);
		float getChargeCurrentLimit();

		float setMaxSystemVoltage(float voltage);
		float getMaxSystemVoltage();

		float setMinSystemVoltage(float voltage);
		float getMinSystemVoltage();

		float setAdapterCurrentLimit2(float current);
		float getAdapterCurrentLimit2();
  
    	float setTricleChargeCurrent(TCCL_t lim);

		bool setNGATE(bool value);

		StateMachineStatus_t getStateMachineStatus();

	private:
		uint8_t _smb_address;

		
};

#endif // ISL9241_H