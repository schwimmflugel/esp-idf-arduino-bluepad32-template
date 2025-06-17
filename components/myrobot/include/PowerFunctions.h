#ifndef POWER_FUNCTIONS_H
#define POWER_FUNCTIONS_H

#include <Arduino.h>

class PowerFunctions {
public:
    PowerFunctions();
    void begin();
    float getBatteryLevel();
    bool checkForLowBattery();

private:
    uint16_t shutdown_voltage;
};

#endif