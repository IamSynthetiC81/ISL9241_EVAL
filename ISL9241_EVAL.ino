#include <Arduino.h>
#include "ISL9241.h"

void BlinkFatal();

#define perror(x) Serial.print("\t");Serial.println(x);
#define perror_fatal(x) perror(x); BlinkFatal();

/*    Test Parameters    */
#define TEST_TIME                 2.0*60


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
#define FREQ                      10                   // Frequency of the program in Hz
                                                        // Change this value to the desired frequency
#define __PERIOD__                (1000.0 / FREQ)

#define PERTRUB_FREQ              80                    // Frequency for Perturbation as 1 action per n samples.
#define PERTRUB_STEP_FACTOR       3                     // Step multiplier
uint64_t nCycles = 0;

/*     Proportional Control  */
#define P                 1.0
#define Step              0.004

/*    Delay definitions      */
#define __T_DEL_LONG__            5000
#define __T_DEL_MID__             1000
#define __T_DEL_SHORT__           100
#define __T_DEL__                 10

/*         Variables        */
float currentLimit                = 0;  // Variable of change
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
 * @param data A float pointer where the data will be saved
 * @param minCurrent The minimum current to start the curve at
 * @param maxCurrent The maximum current to end the curve at
 * @param stepSize The step size to use
 * @param MinVoltage Minimum operating voltage of the board under test.
 * @return Number of data rows, or -1 on error.
 * 
 * @note The first column is the voltage and the second column is the current
 * @note Memory is allocated for the data pointer and is the callers responsibility to free.
 */
int VI_Curve(
  float ***data,
  float minCurrent = 0,
  float maxCurrent = 5.68,
  float stepSize = 0.004,
  float MinVoltage = 5,
  const unsigned int averaging_window = 10
){
  Serial.print(F("minimum current : "));
  Serial.println(minCurrent,3);

  Serial.print(F("maximum current : "));
  Serial.println(maxCurrent,3);

  Serial.print(F("step size : "));
  Serial.println(stepSize,3);

  if (minCurrent < 0 || maxCurrent < 0 || stepSize < 0) {
    perror("Invalid input");
    return -1;
  }

  int length = (maxCurrent - minCurrent) / stepSize;
  if (length < 1) {
    perror("stepSize too large");
    return -1;
  }

  // Allocate a 2xlength block of memory
  *data = (float **)malloc(length * sizeof(float *));
  if (*data == NULL) {
    perror("Memory allocation failed");
    return -1;
  }

  for (int i = 0; i < length; i++) {
    (*data)[i] = (float *)malloc(2 * sizeof(float));
    if ((*data)[i] == NULL) {
      perror("Memory allocation failed");
      return -1;
    }
  }

  int size = -1;

  unsigned int est_time = length * __T_DEL_SHORT__ * 0.001 * averaging_window;
  Serial.print(F("This will take aprox "));
  Serial.print(est_time);
  Serial.println(F(" seconds"));

  /* Scan VI curve  */
  for (int i = 0; i < length; i++) {
    float val = uut.setAdapterCurrentLimit(minCurrent + i * stepSize);

    /*      Takes averages      */
    (*data)[i][0] = 0;
    (*data)[i][1] = 0;
    for (int j = 0; j < averaging_window; j++ ){
      delay(__T_DEL_SHORT__);

      float voltage = uut.getAdapterVoltage();
      float current = uut.getAdapterCurrent();

      (*data)[i][0] += voltage/averaging_window;
      (*data)[i][1] += current/averaging_window;
    }

    size = i + 1;

    if ((*data)[i][0] <= MinVoltage) {break;}

  }
  return size;
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

float algorithm_P_AND_O(float dv, float dp, float currentLimit, float step = 0.004){
  if (dp > 0){
    if(dv > 0){       
      currentLimit += P*SafeGuard*step;
      Serial.print(F("+1,"));
    } else if (dv < 0){ 
      currentLimit -= P*step;
      Serial.print(F("-1,"));
    }else {
      Serial.print(F(" 1,"));
    }
  } else if (dp < 0){
    if(dv > 0){       
      currentLimit -= P*step;
      Serial.print(F("+2,"));
    }
    else if (dv < 0){ 
      currentLimit += P*SafeGuard*step;
      Serial.print(F("-2,"));
    }else {
      Serial.print(F(" 2,"));
    }
  } else {
    Serial.print(F(" 0,"));
    // currentLimit -= P*step;
  }
  return currentLimit;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\t~=~=~=MPPT Charge Controller=~=~=~\n\n");

  /*    ~=~=~=RUN-TIME SANITY CHECKS=~=~=~    */
  if (ISL9241_BATT_NUM < 1 || ISL9241_BATT_NUM > 4){             perror_fatal("Invalid number of batteries");}
  if (BATT_LOWER_VOLTAGE_LIMIT >= BATT_UPPER_VOLTAGE_LIMIT){     perror_fatal("Invalid battery voltage limits");}
  if (BATT_LOWER_VOLTAGE_LIMIT < 2.5){                           perror_fatal("Invalid battery low voltage limit");}
  if (BATT_UPPER_VOLTAGE_LIMIT > ISL9241_BATT_NUM * 4.2){        perror_fatal("Invalid battery upper voltage limit");}
  if (FREQ < 1){                                                 perror_fatal("Invalid frequency");}
  if (!uut.init()){                                              perror_fatal("Init error");}

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
  float maxCurrent = 0.2;
  float minVoltage = ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD;
  unsigned int averaging = 10;

  float** curve;
  int size = VI_Curve(&curve, minCurrent, maxCurrent, stepSize, minVoltage, averaging);
  
  Serial.println(String("Size is -: ") + size);
  if (size <= 0){ perror_fatal("VI curve error");}
  
  for(int i = 0 ; i < size ; i++ ){
    Serial.print(curve[i][0], 3);
    Serial.print("V, ");
    Serial.print(curve[i][1],3);
    Serial.print("A, ");
    Serial.print(i*stepSize,3);
    Serial.print("A\n");
  }
  uut.setAdapterCurrentLimit(0);
  free(curve);

  Serial.println("\n\n\tSETUP COMPLETE !! \n");
  
  Serial.print("Sampling Period : ");
  Serial.println(__PERIOD__);

  startTime = millis();
}

void loop() {
  /* ~=~=~=Maximum Runtime Guard=~=~=~ */
  if ((millis() - startTime) > (TEST_TIME * 1000)) {
    Serial.println("Test Complete");
    BlinkFatal();
  }

  float voltage = uut.getAdapterVoltage();
  float current = uut.getAdapterCurrent(); 

  /* ~=~=~=MIN VOLTAGE GUARD=~=~=~ */
  if (voltage < ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD + 1) {
    SafeGuard = -(voltage - ISL9241_MIN_OPERATING_VOLTAGE) / (ISL9241_MIN_OPERATING_VOLTAGE_THRESHOLD - ISL9241_MIN_OPERATING_VOLTAGE);
  } else {
    SafeGuard = 1;
  }

  float power = voltage*current;
  
  float deltaV = voltage - previousVoltage;
  float deltaP = power - previousPower;

  // Maximum Power Point Tracking (MPPT) algorithm implementation
  // Perturb and Observe (P&O) algorithm
  if(nCycles == PERTRUB_FREQ){
    Serial.print(F(" \nP,"));
    currentLimit += PERTRUB_STEP_FACTOR * Step;
    nCycles = 0;
  } else {
    currentLimit = algorithm_P_AND_O(deltaV,deltaP, currentLimit, Step);
  }

  if (currentLimit < 0) currentLimit = 0.0;     // Lower limit check
  if (currentLimit > 2) currentLimit = 1.0;     // Upper limit check
  uut.setAdapterCurrentLimit(currentLimit);     // Set the new current limit

  previousPower = power;                        // Update the previous power
  previousVoltage = voltage;                    // Update the previous voltage

  // Print values
  Serial.print(voltage, 3);
  Serial.print(",");
  Serial.print(current, 4);
  Serial.print(",");
  Serial.print(power, 3);
  Serial.print(",");
  Serial.print(uut.getBattChargeCurrent(), 3);
  Serial.print(",");
  Serial.print(currentLimit, 3);
  Serial.println("");

  nCycles++;

  // Wait before the next loop iteration
  delay(__PERIOD__);
}
