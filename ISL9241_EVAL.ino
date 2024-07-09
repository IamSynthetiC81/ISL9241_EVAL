#include <Arduino.h>
#include "ISL9241.h"
/*    Test Parameters    */
#define TEST_TIME                 2*60


/*     Number of batteries    */                        
#define ISL9241_BATT_NUM          2                     // Number of batteries in series
                                                        // 2, 3 or 4. Change this value to
                                                        // the number of batteries in series
#if ISL9241_BATT_NUM < 2
  #error "Invalid param ISL9241_BATT_NUM"

#endif
/*   Battery characteristics  */
#define BATT_LOWER_VOLTAGE_LIMIT  2.8                   // Lower voltage limit     
#define BATT_UPPER_VOLTAGE_LIMIT  3.2                   // Upper voltage limit
#define BATT_MAX_CHARGE_CURRENT   1.00                  // Maximum charge current

/*     Program Frequency      */
#define FREQ                      2                     // Frequency of the program in Hz
                                                        // Change this value to the desired frequency
#define __PERIOD__                (1000.0 / FREQ)

/*     Proportional Control  */
#define P                 1

/*    Delay definitions      */
#define __T_DEL_LONG__            5000
#define __T_DEL_MID__             1000
#define __T_DEL_SHORT__           100
#define __T_DEL__                 10

/*         Variables        */
float currentLimit                = 0.01;  // Variable of change
float previousPower               = 0;
float previousVoltage             = 0;
float SafeGuard                   = 1;

bool window                       = false;
unsigned long startTime           = 0;

// Device under test
ISL9241 uut;

/**
 * @brief Blink the LED on the board indefinitely to indicate a fatal error.
 * 
 * @note This function will never return
 */
void BlinkFatal(){
  while(1){
    digitalWrite(LED_BUILTIN,HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN,LOW);
    delay(500);
  }
}

/**
 * @brief Generate a VI curve
 * 
 * @param minCurrent The minimum current to start the curve at
 * @param maxCurrent The maximum current to end the curve at
 * @param stepSize The step size to use
 * @return float** A 2D array of the VI curve
 * 
 * @note The first column is the voltage and the second column is the current
 * @note The caller is responsible for freeing the memory
 */
float **VI_Curve(float minCurrent, float maxCurrent = 5.68, float stepSize = 0.004) {
  Serial.print(F("minimum current : "));
  Serial.println(minCurrent,3);

  Serial.print(F("maximum current : "));
  Serial.println(maxCurrent,3);

  Serial.print(F("step size : "));
  Serial.println(stepSize,3);

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

    delay(__T_DEL__);

    // 10 digit progress bar
    if (i - prgBar > length / 10) {
      prgBar = i;
      Serial.print(F("#"));
    }
  }
  Serial.print("\n");
  return VI_Curve;
}

/** Set up the hardware timer1 to run at a specific frequency
 * @param freq The frequency to run the timer at
 * @return The period of the timer
 */
float initHardwareTimer(uint32_t freq) { 
  // Set the timer in CTC mode
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);

  // Set the prescaler to 64
  TCCR1B |= (1 << CS11) | (1 << CS10);

  // Set the compare value
  OCR1A = (F_CPU / 64 / freq) - 1;

  // Enable the interrupt
  TIMSK1 |= (1 << OCIE1A);

  // Enable global interrupts
  sei();

  return 1.0 / (F_CPU / 64 / freq);
}

/**
 * @brief Timer1 compare match interrupt service routine
 */
ISR(TIMER1_COMPA_vect) {
  window = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\t~=~=~=MPPT Charge Controller=~=~=~\n\n");

  /*    ~=~=~=RUN-TIME SANITY CHECKS=~=~=~    */
  if (ISL9241_BATT_NUM < 1 || ISL9241_BATT_NUM > 4) {
    Serial.println("Invalid number of batteries");
    BlinkFatal();
  }
  if (BATT_LOWER_VOLTAGE_LIMIT >= BATT_UPPER_VOLTAGE_LIMIT) {
    Serial.println("Invalid battery voltage limits");
    BlinkFatal();
  }
  if (BATT_LOWER_VOLTAGE_LIMIT < 2.5 || BATT_UPPER_VOLTAGE_LIMIT > ISL9241_BATT_NUM * 4.2) {
    Serial.println("Invalid battery voltage limits");
    BlinkFatal();
  }
  if (FREQ < 1) {
    Serial.println("Invalid frequency");
    BlinkFatal();
  }
  if (!uut.init()) {
    Serial.println("Init error");
    BlinkFatal();
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

  uut.setChargeCurrentLimit(1.2);  // For SAFETY
  uut.setAdapterCurrentLimit(0.04);


  Serial.println("VI CURVE : ");
  delay(__T_DEL_MID__);

  float stepSize = 0.004;
  float minCurrent = 0;
  float maxCurrent = 1;

  float** curve = VI_Curve(minCurrent, maxCurrent, stepSize);
  size_t curveSize = maxCurrent/stepSize + 1;

  for(int i = 0 ; i < curveSize ; i++ ){
    Serial.print(curve[i][0], 3);
    Serial.print("V, ");
    Serial.print(curve[i][1],3);
    Serial.print("A, ");
    Serial.print(i*stepSize,3);
    Serial.print("A\n");
  }
  free(curve);

  // Serial.println("\n\n\tSETUP COMPLETE !! \n\nPress any button to continue\n");
  // while(1){
  //   if(Serial.available()) break;
  // }
  
  Serial.println("\n\n\tSETUP COMPLETE !! \n");
  
  Serial.print("Sampling Period : ");
  Serial.println(__PERIOD__);

  startTime = millis();
}

void loop() {
  /*    ~=~=~=Maximum Runtime Guard=~=~=~    */
  if(millis() - startTime > TEST_TIME * 1000){
    Serial.println("Test Complete");
    BlinkFatal();
  }

  if (!window) return;                                          // Wait for the timer interrupt

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

  float power = voltage * current;                              // Calculate the power

  // Maximum Power Point Tracking (MPPT) algorithm implemantaion
  // Perturb and Observe (P&O) algorithm
  if (power > previousPower) {
    if (voltage >= previousVoltage) {
      Serial.print("1,"); 
      currentLimit += P*0.004;                                  // Increase the current limit
    } else if (voltage < previousVoltage){
      Serial.print("2,");
      currentLimit -= P*0.004;                                  // Decrease the current limit 
    }
  } else if (power < previousPower) {
    if (voltage >= previousVoltage) {
      Serial.print("-1,");
      currentLimit -= P*0.004;                                  // Decrease the current limit
    } else {
      Serial.print("-2,");
      currentLimit += P*0.004;                                  // Increase the current limit
    }
  } else {
    Serial.print("0,");   
    currentLimit += P*0.004;                                    // Increase the current limit
  }

  if (currentLimit < 0 ) currentLimit = 0.0;                    // Lower limit check
  uut.setAdapterCurrentLimit(currentLimit);                     // Set the new current limit


  previousPower = power;                                        // Update the previous power
  previousVoltage = voltage;                                    // Update the previous voltage

  // Print values
  Serial.print(voltage);
  Serial.print(",");
  Serial.print(current,4);
  Serial.print(",");
  Serial.print(power,4);
  Serial.print(",");
  Serial.print(uut.getBattChargeCurrent(),4);
  Serial.print(",");
  Serial.print(currentLimit,4);
  Serial.println("");

  // Wait before the next loop iteration
  // delay(__PERIOD__);

  window = false;                                               // Reset the window flag
}
