#include "ISL9241.h"

#define CPU_FREQ 16000000

#define PERFORM_VI_CURVE 0
#define RUNTIME -1

/**
 * @brief Blink the LED on the board indefinitely to indicate a fatal error.
 * 
 * @note This function will never return
 */
void BlinkFatal(){
  while(1){
    digitalWrite(13, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(1000);                      // wait for a second
    digitalWrite(13, LOW);   // turn the LED off by making the voltage LOW
    delay(1000);                      // wait for a second
  }
}

#define perror(x) Serial.print("\t");Serial.println(x);
#define perror_fatal(x) perror(x); BlinkFatal();

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
#define BATT_MAX_CHARGE_CURRENT   3.00                  // Maximum charge current
#define BATT_MAX_CHARGE_POWER BATT_UPPER_VOLTAGE_LIMIT*BATT_MAX_CHARGE_CURRENT

/*     Program Frequency      */
#define FREQ                      1                     // Frequency of the program in Hz
                                                        // Change this value to the desired frequency
#define __PERIOD__                (1000.0 / FREQ)

/*     Proportional Control  */
#define P                 1.0
#define Step              0.004

/*    Delay definitions      */
#define __T_DEL_LONG__            5000
#define __T_DEL_MID__             1000
#define __T_DEL_SHORT__           20
#define __T_DEL__                 10

/*         Variables        */
float currentLimit                = 0;  // Variable of change
float previousPower               = 0;
float previousVoltage             = 0;
float previousCurrent             = 0;
float previousBatteryCurrent      = 0;
float SafeGuard                   = 1;
float maxPower                    = 0;

volatile bool window              = false;
unsigned long startTime           = 0;

enum BatteryState_t{
  CHARGING,
  DISCHARGING,
  FULL
};

// Device under test
ISL9241 uut;

float adaptiveStep(float dP, float baseStep = 0.004, float scale = 1) {
  return baseStep + abs(dP) * scale;
}

float smoothData(float newValue, float oldValue, float alpha = 0.5) {
  return alpha * newValue + (1 - alpha) * oldValue;
}

float averageVoltageMeasurement(int n = 10){
  float sum = 0;
  for (int i = 0; i < n; i++){
    float val = uut.getAdapterVoltage();
    if (val == -1) return -1;

    sum += val;
    delay(1);
  }
  return sum / n;
}

float averageCurrentMeasurement(int n = 10){
  float sum = 0;
  for (int i = 0; i < n; i++){
    float val = uut.getAdapterCurrent();
    if (val == -1) return -1;

    sum += val;
    delay(1);
  }
  return sum / n;
}

/**
 * @brief Configure the timer to generate an interrupt every period
 * 
 * @param period_ms The period of the timer in milliseconds
 */
void TimerPeriodic(uint16_t period_ms){
    // Disable global interrupts
    cli();
    
    // Reset Timer1 registers
    TCCR1A = 0; // Normal mode
    TCCR1B = 0; // Timer stopped
    TCNT1 = 0;  // Clear timer counter

    // Calculate the value for OCR1A
    // Clock frequency = 16 MHz
    // Prescaler options: 1, 8, 64, 256, 1024
    // Formula: OCR1A = (F_CPU / (prescaler * frequency)) - 1
    // Frequency = 1000 / period_ms
    uint32_t compare_match;
    uint16_t prescaler = 64; // A reasonable prescaler
    compare_match = (F_CPU / (prescaler * 1000UL / period_ms)) - 1;

    // Ensure compare match value fits within 16-bit range
    if (compare_match > 65535) {
        // Adjust prescaler if needed
        prescaler = 256;
        compare_match = (F_CPU / (prescaler * 1000UL / period_ms)) - 1;
    }

    if (compare_match > 65535) {
        // Maximum possible period with prescaler 256 is 1048 ms
        prescaler = 1024;
        compare_match = (F_CPU / (prescaler * 1000UL / period_ms)) - 1;
    }

    OCR1A = (uint16_t)compare_match; // Set compare match value

    // Set the prescaler
    switch (prescaler) {
        case 1:
            TCCR1B |= (1 << CS10); // No prescaler
            break;
        case 8:
            TCCR1B |= (1 << CS11); // Prescaler 8
            break;
        case 64:
            TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
            break;
        case 256:
            TCCR1B |= (1 << CS12); // Prescaler 256
            break;
        case 1024:
            TCCR1B |= (1 << CS12) | (1 << CS10); // Prescaler 1024
            break;
    }

    // Configure Timer1 in CTC mode
    TCCR1B |= (1 << WGM12); // WGM12: CTC mode

    // Enable Timer1 compare match interrupt
    TIMSK1 |= (1 << OCIE1A);

    // Enable global interrupts
    sei();
}

/**
 * @brief Timer 1 compare interrupt service routine
 * 
 * @note This function is called every period
 */
ISR(TIMER1_COMPA_vect){
  window = true;
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
  float minCurrent = 0.04,
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

  Serial.print(F("Length is : "));
  Serial.println(length);

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

  uut.setAdapterCurrentLimit(minCurrent);
  Serial.print(F("Current limit set to : "));
  Serial.println(minCurrent, 3);
  Serial.print(F("Please, restart the electronic load\n The test will initiate in 5 seconds\n"));
  delay(__T_DEL_LONG__);
  
  /* Scan VI curve  */
  for (int i = 0; i < length; i++) {
    float val = uut.setAdapterCurrentLimit(minCurrent + i * stepSize);

    /*      Takes averages      */
    (*data)[i][0] = 0;
    (*data)[i][1] = 0;
    
    (*data)[i][0] = averageVoltageMeasurement(averaging_window);
    (*data)[i][1] = averageCurrentMeasurement(averaging_window);
    
    size = i + 1;

    if ((*data)[i][0] <= MinVoltage) {break;}

    delay(__T_DEL_SHORT__);
  }
  return size;
}


float MPPT(float previousVoltage, float voltage, float previousCurrent, float current, float currentLimit, float step = 0.004){
  float dV = voltage - previousVoltage;
  float dI = current - previousCurrent;
  float dP = voltage * current - previousVoltage * previousCurrent;

  Serial.print(dV, 3);
  Serial.print(F(" , "));
  Serial.print(dI, 3);
  Serial.print(F(" , "));
  Serial.print(dP, 3);
  Serial.print(F(" , "));

  if (voltage <= 4.0) {
    return currentLimit - step;
  }

  if (dV == 0) {
    if (dP = 0){
      return currentLimit + step;
    } else {
      return currentLimit - step;
    }
  } else if (dV < 0){
    if (dP > 0){
      return currentLimit + step;
    } else {
      return currentLimit - step;
    }
  } else {
    if (dP = 0){
      return currentLimit + step;
    } else {
      return currentLimit - step;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\t~=~=~=MPPT Charge Controller=~=~=~\n\n");

  uut = ISL9241();

  /*    ~=~=~=RUN-TIME SANITY CHECKS=~=~=~    */
  if (ISL9241_BATT_NUM < 1 || ISL9241_BATT_NUM > 4){             perror_fatal("Invalid number of batteries");}
  if (BATT_LOWER_VOLTAGE_LIMIT >= BATT_UPPER_VOLTAGE_LIMIT){     perror_fatal("Invalid battery voltage limits");}
  if (BATT_LOWER_VOLTAGE_LIMIT < 2.5){                           perror_fatal("Invalid battery low voltage limit");}
  if (BATT_UPPER_VOLTAGE_LIMIT > ISL9241_BATT_NUM * 4.2){        perror_fatal("Invalid battery upper voltage limit");}
  if (FREQ < 1){                                                 perror_fatal("Invalid frequency");}
  if (!uut.init()){                                              perror_fatal("Init error");}



  if(!uut.writeBit(Control0, 0, 0)){   perror("Could not disable Reverse Turbo Mode")}              // Disable Reverse Turbo Mode
  if(!uut.writeBit(Control0, 1, 0)){   perror("Could not write bit")}                               // 
  if(!uut.writeBit(Control3, 5, 0)){   perror("Could not enable Input Current Limit Control");}     // Enable Input Current Limit Control
  if(!uut.writeBit(Control3, 7, 0)){   perror("Could not enable Autocharging");}                    // ChargeControl to AutoCharging
  if(!uut.writeBit(Control3, 7, 1)){   perror("Could not enable Auto charging");}                   // Enable Auto-Charging
  if(!uut.writeBit(Control2, 12, 0)){  perror("Could not disable two level adapter control");}      // Disable two level adapter current limit
  

  uut.setMaxSystemVoltage(ISL9241_BATT_NUM * BATT_UPPER_VOLTAGE_LIMIT);
  uut.setMinSystemVoltage(ISL9241_BATT_NUM * BATT_LOWER_VOLTAGE_LIMIT);

  uut.setTricleChargeCurrent(TCCL_t::TC_256mA);

  uut.setChargeCurrentLimit(1.2);  // For SAFETY
  uut.setAdapterCurrentLimit(0.04);


  if(PERFORM_VI_CURVE){
    Serial.println("Performing VI curve");
    delay(__T_DEL_MID__);
    float** curve;
    int size = VI_Curve(&curve, 0, 2, 0.004, 4, 10);
    if (size <= 0){ perror_fatal("VI curve error");}
    for(int i = 0 ; i < size ; i++ ){
      Serial.print(curve[i][0], 3);
      Serial.print("V, ");
      Serial.print(curve[i][1],3);
      Serial.print("A, ");
      Serial.print(i*0.004,3);
      Serial.print("A\n");
    }
    free(curve);
  }
  
  uut.setAdapterCurrentLimit(0.1);

  Serial.println("\n\n\tSETUP COMPLETE !! \n");
  
  Serial.print("Sampling Period : ");
  Serial.println(__PERIOD__);

  delay(500);

  startTime = millis();

  previousVoltage = averageVoltageMeasurement();
  previousCurrent = averageCurrentMeasurement();
  previousPower = previousVoltage * previousCurrent;

  delay(500);

  TimerPeriodic(__PERIOD__);
}

void loop() {
  if (!window){
    return;
  }

  #if RUNTIME > 0
    /* ~=~=~=Maximum Runtime Guard=~=~=~ */
    if ((millis() - startTime) > (RUNTIME * 1000)) {
      Serial.println("Test Complete");
      BlinkFatal();
    }
  #endif

  float voltage = averageVoltageMeasurement();
  float current = averageCurrentMeasurement();
  float power = voltage * current;

  // timestap 
  Serial.print(millis() - startTime);
  Serial.print(",");

  float step = adaptiveStep(power - previousPower, Step, 100);
  currentLimit = MPPT(previousVoltage, voltage, previousCurrent, current, currentLimit, 0.004);

  if (currentLimit < 0.1) currentLimit = 0.1;     // Lower limit check
  if (currentLimit > 2) currentLimit = 2.0;     // Upper limit check

  uut.setAdapterCurrentLimit(currentLimit);     // Set the new current limit

  previousPower = power;                        // Update the previous power
  previousVoltage = voltage;                    // Update the previous voltage
  previousCurrent = current;                    // Update the previous current

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

  // nCycles++;

  // Wait before the next loop iteration
  delay(__PERIOD__);
}