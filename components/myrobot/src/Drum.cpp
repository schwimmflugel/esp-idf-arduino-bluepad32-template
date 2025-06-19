#include "Drum.h"
#include "Constants.h"
#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "Drum";

Drum::Drum(uint8_t pwmPin) : _pwmPin(pwmPin) {
    setInputLimits();
}

void Drum::begin() {
    // Initialize the PWM channel with the desired frequency and resolution
    ledcAttach(_pwmPin, ESC_PWM_FREQ, ESC_PWM_RESOLUTION);

    //Caluclate the maximuum value that may be sent given the resolution of the PWM
    maxPwmVal = (1 << ESC_PWM_RESOLUTION)-1;

    // Initialize ESC with a neutral signal
    setSpeed(0);
}


/**
 * Sets the minimum and maximum expected input values that will be recieved from the controller.
 * Default: 0 to 1023
 * @param minInput      Minimum input value from the controller
 * @param maxInput      Maximum input value from the controller
 */
void Drum::setInputLimits(int minInput, int maxInput){
    _minInput = minInput;
    _maxInput = maxInput;
}

//Function to initialize the ESC on start up
//The function is non-blocking so must be performed in a loop and returns True when complete
bool Drum::initializeESC(){
    static bool initializationStarted = false;
    static uint32_t start_time;
    if( initializationStarted == false){
        initializationStarted = true;
        setSpeed(maxPwmVal/ESC_INITIALIZE_FRACTION);
        start_time = millis();
    }
    if( millis() - start_time >= ESC_INITIALIZE_WAIT_TIME && initializationStarted == true){
        setSpeed(0);
        initializationStarted = false;
    }
    return true;
}

/**
 * Sets the drum speed with the input from the controller value
 * @param speedInput     The speed input value between Min and Max Input
 */
void Drum::setSpeed(int16_t speedInput) {
    // Ensure throttlePercent is within the valid range
    speedInput = constrain(speedInput, _minInput, _maxInput);

    // Map the speed value to the appropriate pulsewidth 
    uint16_t pulseWidthUs = map(speedInput, _minInput, _maxInput, ESC_MIN_PULSEWIDTH, ESC_MAX_PULSEWIDTH);

    //Convert the pulse width in µs to a duty cycle for the given frequency that is set
    uint16_t duty_cycle = (pulseWidthUs * maxPwmVal) / (1000000 / ESC_PWM_FREQ);
    ESP_LOGD(TAG, "ESC Duty Cycle: %d\tESC Pulse Width: %d uSec", duty_cycle, pulseWidthUs);
    

    // Use ledcWriteMicroseconds to send the PWM signal
    ledcWrite(ESC_1_PIN, duty_cycle);
}

//Stop the drum by sending the minimum pulse width signal.
//Stopping the signal altogether would not stop the ESC from it's last recieved value
void Drum::stop(){
    setSpeed(_minInput);
}
