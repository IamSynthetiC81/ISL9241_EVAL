#include "src/ISL9241.h"

#include <avr/io.h>
#include <avr/interrupt.h>


#define CPU_FREQ 16000000

#define PERFORM_VI_CURVE 1
unsigned long RUNTIME = 120;

/**
 * @brief Blink the LED on the board indefinitely to indicate a fatal error.
 * 
 * @note This function will never return
 */
void BlinkFatal(){
  // Set the LED pin as output
  pinMode(13, OUTPUT);

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

/*     Program Frequency      */
#define FREQ                      5                     // Frequency of the program in Hz
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
float previousPower               = 0;  // Previous power
float dP                          = 0;  // Change in power
float previousdP                  = 0;  // Gradient of power
float previousVoltage             = 0;
float previousCurrent             = 0;
float previousBatteryCurrent      = 0;
float SafeGuard                   = 1;

volatile bool window              = false;
volatile bool SupplyCorrection    = false;
unsigned long startTime           = 0;


// Device under test
ISL9241 uut;

/**
 * @brief Τake n measurements and return the average value
 * 
 * @param n The number of measurements to take
 * @param delay_ms The delay between measurements in milliseconds
 * @return float The average value of the measurements
 */
float averageVoltageMeasurement(int n = 10,  unsigned int delay_ms = 1){
  float sum = 0;
  for (int i = 0; i < n; i++){
    float val = uut.getAdapterVoltage();
    if (val == -1) return -1;

    sum += val;

    delay(delay_ms);
    
  }
  return sum / n;
}

/**
 * @brief Take n measurements and return the average value
 * 
 * @param n The number of measurements to take
 * @param delay_ms The delay between measurements in milliseconds
 * @return float The average value of the measurements
 */
float averageCurrentMeasurement(int n = 10,  unsigned int delay_ms = 1){
  float sum = 0;
  for (int i = 0; i < n; i++){
    float val = uut.getAdapterCurrent();
    if (val == -1) return -1;

    sum += val;
    
    delay(delay_ms);
  }
  return sum / n;
}

/**
 * @brief Configure a timer to generate an interrupt every period
 * 
 * @param period_ms The period of the timer in milliseconds
 * @warning The user must define his own ISR for the timer
 * @note this function is applicable for Timer1, Timer3, Timer4 and Timer5 only
 */
bool Timer_Init(uint8_t timer_number, uint16_t period_ms) {
    // Disable global interrupts
    cli();

    // Pointers to timer-specific registers
    volatile uint8_t *tccra, *tccrb, *timsk;
    volatile uint16_t *ocr, *tcn;

    // Determine which timer to configure
    switch (timer_number) {
        case 1:
            tccra = &TCCR1A;
            tccrb = &TCCR1B;
            timsk = &TIMSK1;
            ocr = &OCR1A;
            tcn = &TCNT1;
            break;
        case 3:
            tccra = &TCCR3A;
            tccrb = &TCCR3B;
            timsk = &TIMSK3;
            ocr = &OCR3A;
            tcn = &TCNT3;
            break;
        case 4:
            tccra = &TCCR4A;
            tccrb = &TCCR4B;
            timsk = &TIMSK4;
            ocr = &OCR4A;
            tcn = &TCNT4;
            break;
        case 5:
            tccra = &TCCR5A;
            tccrb = &TCCR5B;
            timsk = &TIMSK5;
            ocr = &OCR5A;
            tcn = &TCNT5;
            break;
        default:
            // Invalid timer number, return
            return;
    }

    // Reset timer registers
    *tccra = 0; // Normal mode
    *tccrb = 0; // Timer stopped
    *tcn = 0;   // Clear timer counter
    *ocr = 0;   // Clear compare match value
    *timsk = 0; // Disable all interrupts


    // Calculate the value for OCRnA
    // Clock frequency = 16 MHz
    // Prescaler options: 1, 8, 64, 256, 1024
    // Formula: OCRnA = (F_CPU / (prescaler * frequency)) - 1
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

    if (compare_match > 65535) {
        // Maximum possible period with prescaler 1024 is 4194 ms
        // If compare match value is still too large, return
        return false;
    }

    *ocr = (uint16_t)compare_match; // Set compare match value

    // Set the prescaler
    switch (prescaler) {
        case 1:
            *tccrb |= (1 << CS10); // No prescaler
            break;
        case 8:
            *tccrb |= (1 << CS11); // Prescaler 8
            break;
        case 64:
            *tccrb |= (1 << CS11) | (1 << CS10); // Prescaler 64
            break;
        case 256:
            *tccrb |= (1 << CS12); // Prescaler 256
            break;
        case 1024:
            *tccrb |= (1 << CS12) | (1 << CS10); // Prescaler 1024
            break;
    }

    // Configure timer in CTC mode
    *tccrb |= (1 << WGM12); // WGM12: CTC mode

    switch (timer_number) {
      case 1:
        *timsk |= (1 << OCIE1A); // Timer1 Compare Match A Interrupt
        break;
      case 3:
        *timsk |= (1 << OCIE3A); // Timer3 Compare Match A Interrupt
        break;
      case 4:
        *timsk |= (1 << OCIE4A); // Timer4 Compare Match A Interrupt
        break;
      case 5:
        *timsk |= (1 << OCIE5A); // Timer5 Compare Match A Interrupt
        break;
      default:
        return; // Invalid timer, do nothing
    }


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

  float voltage, current, batteryVoltage, batteryCurrent;

  /* Scan VI curve  */
  for (int i = 0; i < length; i++) {
    float val = uut.setAdapterCurrentLimit(minCurrent + i * stepSize);
    
    voltage = averageVoltageMeasurement(averaging_window);
    current = averageCurrentMeasurement(averaging_window);
    batteryVoltage = uut.getBatteryVoltage();
    batteryCurrent = uut.getBattChargeCurrent();
    
    if (voltage == -1 || current == -1 || batteryVoltage == -1 || batteryCurrent == -1) {
      perror("Error in measurement");
      return -1;
    }

    if (voltage <= MinVoltage) {break;}

    Serial.print(voltage, 3);
    Serial.print(F(" , "));
    Serial.print(current, 3);
    Serial.print(F(" , "));
    Serial.print(batteryVoltage, 3);
    Serial.print(F(" , "));
    Serial.print(batteryCurrent, 3);
    Serial.print(F(" , "));
    Serial.print(val, 3);
    Serial.println(F(""));


    delay(200);
  }
}


float MPPT(float previousVoltage, float voltage, float previousCurrent, float current, float currentLimit, float step = 0.004){
  float dV = voltage - previousVoltage;
  dV = abs(dV) < 0.001 ? 0 : dV;

  float dI = current - previousCurrent;
  dI = abs(dI) < 0.001 ? 0 : dI;
  dP = voltage * current - previousVoltage * previousCurrent;
  

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
    if (dP >= 0){
      if (dI >= 0){
        return currentLimit + step;
      } else {
        return currentLimit - step;
      }
    } else {
      return currentLimit + step;
    }
  } else if (dV < 0){
    if (dP > -0.01){
      return currentLimit + step;
    } else {
      if (dI > 0){ // no >=  0
        return currentLimit - step;
      } else {
        return currentLimit + step;
      }
    }
  } else {
    if (dP > 0){
      if (dI > 0){
        return currentLimit + step;
      } else {
        return currentLimit - step;
      }
    } else {
      return currentLimit + step;
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

  Serial.print("Runtime : ");
  Serial.print(RUNTIME*1000);
  Serial.println(" milliseconds");
  

  if(!uut.writeBit(Control0, 0, 0)){   perror("Could not disable Reverse Turbo Mode")}              // Disable Reverse Turbo Mode
  if(!uut.writeBit(Control0, 1, 0)){   perror("Could not write bit")}                               // 
  if(!uut.writeBit(Control3, 5, 0)){   perror("Could not enable Input Current Limit Control");}     // Enable Input Current Limit Control
  if(!uut.writeBit(Control3, 7, 0)){   perror("Could not enable Autocharging");}                    // ChargeControl to AutoCharging
  if(!uut.writeBit(Control3, 7, 1)){   perror("Could not enable Auto charging");}                   // Enable Auto-Charging
  if(!uut.writeBit(Control2, 12, 0)){  perror("Could not disable two level adapter control");}      // Disable two level adapter current limit
  

  uut.setMaxSystemVoltage(ISL9241_BATT_NUM * BATT_UPPER_VOLTAGE_LIMIT);
  uut.setMinSystemVoltage(ISL9241_BATT_NUM * BATT_LOWER_VOLTAGE_LIMIT);

  uut.setTricleChargeCurrent(TCCL_t::TC_256mA);

  uut.setChargeCurrentLimit(BATT_MAX_CHARGE_CURRENT);  // For SAFETY
  uut.setAdapterCurrentLimit(0.004);


  if(PERFORM_VI_CURVE){
    Serial.println("Performing VI curve");
    delay(__T_DEL_MID__);
    VI_Curve(0, 2, 0.004, 4, 10);
  }
  
  uut.setAdapterCurrentLimit(0.1);

  Serial.println("\n\n\tSETUP COMPLETE !! \n");
  
  Serial.print("Sampling Period : ");
  Serial.println(__PERIOD__);

  delay(500);

  previousVoltage = averageVoltageMeasurement();
  previousCurrent = averageCurrentMeasurement();
  previousPower = previousVoltage * previousCurrent;

  delay(500);

  Timer_Init(1,__PERIOD__);
  startTime = millis();
}

void loop() {
  /* ~=~=~=Maximum Runtime Guard=~=~=~ */
  if (millis() - startTime > RUNTIME * 1000){ 
    Serial.println("Test Complete");
    BlinkFatal();
  }
  

  if (!window){
    return;
  }

  

  float voltage = averageVoltageMeasurement();
  float current = averageCurrentMeasurement();
  float power = voltage * current;

  float V_OUTP = averageAnalogRead(A0)*V_SUPPLY/1023.0;
  float I_OUTP = (V_OUTP-V_SUPPLY/2) / 0.185;
  // timestap 
  Serial.print(millis() - startTime);
  Serial.print(", ");

  currentLimit = MPPT(previousVoltage, voltage, previousCurrent, current, currentLimit, Step);

  if (currentLimit < 0.1) currentLimit = 0.1;                                             // Lower limit check
  if (currentLimit > BATT_MAX_CHARGE_CURRENT) currentLimit = BATT_MAX_CHARGE_CURRENT;     // Upper limit check

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
  Serial.print(",");
  Serial.print(I_OUTP, 3);
  Serial.println("");
}