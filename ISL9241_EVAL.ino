#include <Arduino.h>
#include "ISL9241.h"

#define ISL9241_BATT_NUM 2

#define BATT_LOWER_VOLTAGE_LIMIT 2.8
#define BATT_UPPER_VOLTAGE_LIMIT 3.2

#define FREQ 10
#define __PERIOD__ (1000.0 / FREQ)

#define P 5

// Variables
float previousPower = 0;
float previousVoltage = 0;
float SafeGuard = 1;

ISL9241 uut;


float **VI_Curve(float minCurrent, float maxCurrent, float stepSize = 0.004) {
  if (minCurrent < 0 || maxCurrent < 0 || stepSize < 0) {
    Serial.println("Invalid input");
    return nullptr;
  }

  int length = (maxCurrent - minCurrent) / stepSize;
  if (length < 1) {
    Serial.println("stepSize too large");
    return nullptr;
  }


  // Allocate a 2xlength block of memory
  float **VI_Curve;
  VI_Curve = (float **)malloc(length * sizeof(float *));
  if (VI_Curve == NULL) {
    Serial.println("Memory allocation failed");
    return nullptr;
  }

  for (int i = 0; i < length; i++) {
    VI_Curve[i] = (float *)malloc(2 * sizeof(float));
    if (VI_Curve[i] == NULL) {
      Serial.println("Memory allocation failed");
      return nullptr;
    }
  }

  int prgBar = 0;
  Serial.print(F("/..........\\\n/"));

  /* Scan VI curve  */
  for (int i = 0; i < length; i++) {
    uut.setAdapterCurrentLimit(minCurrent + i * stepSize);
    VI_Curve[i][0] = uut.getAdapterVoltage();
    VI_Curve[i][1] = uut.getAdapterCurrent();

    delay(10);

    // 10 digit progress bar
    if (i - prgBar > length / 10) {
      prgBar = i;
      Serial.print(F("#"));
      Serial.print("\b$");
    }
  }

  Serial.println(F("\\ \n D O N E !\n"));

  return VI_Curve;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Booting up ...");

  /*    ~=~=~=RUN-TIME SANITY CHECKS=~=~=~    */
  if (ISL9241_BATT_NUM < 1 || ISL9241_BATT_NUM > 4) {
    Serial.println("Invalid number of batteries");
    // while(1);
  }
  if (BATT_LOWER_VOLTAGE_LIMIT >= BATT_UPPER_VOLTAGE_LIMIT) {
    Serial.println("Invalid battery voltage limits");
    // while(1);
  }
  if (BATT_LOWER_VOLTAGE_LIMIT < 2.5 || BATT_UPPER_VOLTAGE_LIMIT > ISL9241_BATT_NUM * 4.2) {
    Serial.println("Invalid battery voltage limits");
    // while(1);
  }
  if (FREQ < 1) {
    Serial.println("Invalid frequency");
    // while(1);
  }

  if (!uut.init()) {
    Serial.println("Init error");
  }

  uut.writeBit(Control0, 0, 0);   // Disable Reverse Turbo Mode
  uut.writeBit(Control0, 1, 0);   //
  uut.writeBit(Control3, 5, 0);   // Enable Input Current Limit Control
  uut.writeBit(Control3, 7, 0);   // ChargeControl to AutoCharging
  uut.writeBit(Control2, 12, 0);  // Disable two level adapter current limit
  uut.writeBit(Control3, 7, 1);   // Enable Auto-Charging

  uut.setMaxSystemVoltage(ISL9241_BATT_NUM * BATT_UPPER_VOLTAGE_LIMIT);
  uut.setMinSystemVoltage(ISL9241_BATT_NUM * BATT_LOWER_VOLTAGE_LIMIT);

  uut.setTricleChargeCurrent(TCCL_t::TC_256mA);

  uut.setChargeCurrentLimit(3);  // For SAFETY
  uut.setChargeCurrentLimit(5.68);
  uut.setAdapterCurrentLimit(0.04);


  Serial.println("Starting...");
  delay(1000);

  float** curve = VI_Curve(0,0.6);
  size_t curveSize = 5.68/0.04;

  for(int i = 0 ; i < curveSize ; i++ ){
    Serial.println(curve[i][0] + String("V, ") + curve[i][1] + String("A"));
  }
  free(curve);

  Serial.println("\n\n\tIt is DONE !! \n\n");


  delay(5000);
}

void loop() {
  float voltage = uut.getAdapterVoltage();
  float current = uut.getAdapterCurrent(); 

  // while (voltage < ISL9241_MIN_OPERATING_VOLTAGE) {
  //   //@TODO: At this point, the ISL9241 will be unreachable
  //   uut.setChargeCurrentLimit(0);  // Stop charging

  //   delay(1000);
  //   voltage = uut.getAdapterVoltage();

  //   Serial.println(String("LOW OPERATING VOLTAGE : ") + voltage + String("V"));
  // }

  /*    ~=~=~=MIN VOLTAGE GUARD=~=~=~    */
  if (voltage > ISL9241_MIN_OPERATING_VOLTAGE && voltage < ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD) {
    SafeGuard = (voltage - ISL9241_MIN_OPERATING_VOLTAGE) / (ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD - ISL9241_MIN_OPERATING_VOLTAGE);
  } else {
    SafeGuard = 1;
  }

  // Calculate power
  float power = voltage * current;

  // Maximum Power Point Tracking (MPPT) algorithm implemantaion
  uint16_t currentLimit;  // Variable of change

  uut.readRegister(AdapterCurrentLimit1, &currentLimit);

  // Perturb and Observe (P&O) algorithm
  if (power >= previousPower) {
    if (voltage >= previousVoltage) {
      Serial.print("1,");

      currentLimit += SafeGuard*P*(1 << 2);  // Increase charge current
      // currentLimit += (1 << 2);
    } else {
      Serial.print("2,");

      currentLimit -= SafeGuard*P*(1 << 2);  // Decrease charge current
      // currentLimit -= (1 << 2);
    }
  } else {
    if (voltage >= previousVoltage) {
      Serial.print("-1,");

      currentLimit += SafeGuard*P*(1 << 2);  // Decrease charge current
      // currentLimit += (1 << 2);
    } else {
      Serial.print("-2,");

      currentLimit -= SafeGuard*P*(1 << 2);  // Increase charge current
      // currentLimit -= (1 << 2);
    }
  }

  // Update the charge current limit
  currentLimit = uut.writeRegister(AdapterCurrentLimit1, currentLimit);

  // Serial.print(SafeGuard);
  // Serial.print(",");
  // Serial.print(power);
  // Serial.print(",");
  // Serial.print(currentLimit);


  // Save the current values for the next iteration
  previousPower = power;
  previousVoltage = voltage;

  // Print values
  Serial.print(voltage);
  Serial.print(",");
  Serial.print(current);
  Serial.print(",");
  Serial.print(power);
  Serial.println("");

  // Wait before the next loop iteration
  delay(__PERIOD__);
}
