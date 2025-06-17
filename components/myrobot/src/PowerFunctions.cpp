#include "PowerFunctions.h"
#include "Constants.h"

PowerFunctions::PowerFunctions(){}

void PowerFunctions::begin() {
    //Calculate the shutdown voltage to not go below the minimum cell voltage times the number of cells
    analogReadResolution(12); // Set ADC resolution to 12-bit
    shutdown_voltage = MIN_MVOLT_PER_CELL * NUM_OF_CELLS;
    Serial.print("Shutdown Voltage Set: ");
    Serial.println(shutdown_voltage);
}


float PowerFunctions::getBatteryLevel() {
    uint32_t rawValues;
    for(uint8_t sample = 0; sample < BATT_SAMPLE_COUNT; sample++){
        rawValues += analogRead(BATT_MEAS_PIN);
    }

    //Battery voltage in mVolts
    float batteryVoltage = (rawValues / BATT_SAMPLE_COUNT) * BATTERY_MULTIPLIER * (3.3 / 4095.0) * 1000;
    Serial.print("Battery Voltage: ");
    Serial.println(batteryVoltage);

    return batteryVoltage;
}

//Function to monitor the battery level. Returns True if battery level is low, False if otherwise
//This has a specific read frequency so it should be called regularly but will only update as define in: BATT_READ_FREQ
bool PowerFunctions::checkForLowBattery(){
    
    static bool lastUpdatedValue = false; //Variable to track the bool value between reads
    static u32_t lastBatteryCheckTime;

    //If within the update period, check for a low battery
    if(millis() - lastBatteryCheckTime >= BATT_READ_FREQ){
        lastBatteryCheckTime = millis();
        if( getBatteryLevel() <= shutdown_voltage ){
            Serial.println("Low Power");
            lastUpdatedValue = true;
        }
        else{
            lastUpdatedValue = false;
        }
    }
    return lastUpdatedValue;
}
