# ISL9241 Library

Η βιβλιοθήκη ISL9241 παρέχει μια διεπαφή για τον έλεγχο του ISL9241, ενός ολοκληρωμένου κυκλώματος διαχείρισης μπαταρίας, μέσω του πρωτοκόλλου SMBus. Αυτή η βιβλιοθήκη είναι σχεδιασμένη για χρήση με την πλατφόρμα Arduino.

## Εγκατάσταση

Για να εγκαταστήσετε τη βιβλιοθήκη, αντιγράψτε τα αρχεία [`ISL9241.h`](ISL9241.h) και [`ISL9241.cpp`](ISL9241.cpp) στον φάκελο της βιβλιοθήκης του Arduino.

## Χρήση

```cpp
#include "ISL9241.h"

#define _CURRENT_LIMIT_MIN_ 0.0                             // Μπορεί να οριστεί από τον χρήστη
#define _CURRENT_LIMIT_MAX_ 3.0                             // Μπορεί να οριστεί από τον χρήστη
#define _CURRENT_LIMIT_STEP_ 0.1                            // Μπορεί να οριστεί από τον χρήστη - το μικρότερο βήμα που μπορεί να πάρει είναι 0.004Α */

ISL9241 uut;

float previousVoltage = 0, previousCurrent = 0;
float currentLimit = _CURRENT_LIMIT_MIN_;                   

void setup() {
    Serial.begin(115200);
    uut.init();
}

void loop() {
    // Ο κώδικας που ακολουθεί είναι ένα παράδειγμα χρήσης της βιβλιοθήκης ISL9241.
    float voltage = uut.getAdapterVoltage();
    float current = uut.getAdapterCurrent();

    currentLimit = MPPT(previousVoltage, voltage, previousCurrent, , current, currentLimit, _CURRENT_LIMIT_STEP_);

    currentLimit = (currentLimit < _CURRENT_LIMIT_MIN_) ? _CURRENT_LIMIT_MIN_ : currentLimit;
    currentLimit = (currentLimit > _CURRENT_LIMIT_MAX_) ? _CURRENT_LIMIT_MAX_ : currentLimit;

    uut.setAdapterCurrentLimit(currentLimit);
}
```

## Συναρτήσεις
- `ISL9241(uint8_t read_address)` Κατασκευαστής της κλάσης ISL9241.
- `bool init(unsigned int NoOfCells, float minBattVoltage, float  maxBattVoltage)`: Αρχικοποιεί τη συσκευή.
- `bool writeRegister(uint16_t reg, uint16_t value)`: Γράφει μια τιμή σε ένα καταχωρητή.
- `bool readRegister(uint16_t reg, uint16_t *value)`: Διαβάζει μια τιμή από ένα καταχωρητή.
- `bool writeBit(uint16_t reg, uint8_t bit, bool value)`: Γράφει ένα bit σε ένα καταχωρητή.
- `bool readBit(uint16_t reg, uint8_t bit)`: Διαβάζει ένα bit από ένα καταχωρητή.
- `float SetSysVoltage(float voltage)`: Ρυθμίζει την τάση του συστήματος.
- `float GetSysVoltage()`: Διαβάζει την τάση του συστήματος.
- `float getBattDischargeCurrent()`: Διαβάζει το ρεύμα εκφόρτισης της μπαταρίας.
- `float getBattChargeCurrent()`: Διαβάζει το ρεύμα φόρτισης της μπαταρίας.
- `float getAdapterVoltage()`: Διαβάζει την τάση του μετατροπέα.
- `float getAdapterCurrent()`: Διαβάζει το ρεύμα του μετατροπέα.
- `float getBatteryVoltage()`: Διαβάζει την τάση της μπαταρίας.
- `float setAdapterCurrentLimit(float current)`: Ρυθμίζει το όριο ρεύματος του μετατροπέα.
- `float getAdapterCurrentLimit()`: Διαβάζει το όριο ρεύματος του μετατροπέα.
- `float setChargeCurrentLimit(float current)`: Ρυθμίζει το όριο ρεύματος φόρτισης.
- `float getChargeCurrentLimit()`: Διαβάζει το όριο ρεύματος φόρτισης.
- `float setMaxSystemVoltage(float voltage)`: Ρυθμίζει τη μέγιστη τάση του συστήματος.
- `float getMaxSystemVoltage()`: Διαβάζει τη μέγιστη τάση του συστήματος.
- `float setMinSystemVoltage(float voltage)`: Ρυθμίζει την ελάχιστη τάση του συστήματος.
- `float getMinSystemVoltage()`: Διαβάζει την ελάχιστη τάση του συστήματος.
- `float setAdapterCurrentLimit2(float current)`: Ρυθμίζει το δεύτερο όριο ρεύματος του - μετατροπέα.
- `float getAdapterCurrentLimit2()`: Διαβάζει το δεύτερο όριο ρεύματος του μετατροπέα.
- `float setTricleChargeCurrent(TCCL_t lim)`: Ρυθμίζει το όριο ρεύματος φόρτισης trickle.
