#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

//Modifiable Values
#define WEAPON_ENABLE true

#define DRIVE_MOTOR_PWM_FREQ 40000
#define DRIVE_MOTOR_PWM_RESOLUTION 8

#define ESC_PWM_FREQ 2000
#define ESC_PWM_RESOLUTION 8
#define ESC_MIN_PULSEWIDTH 125
#define ESC_MAX_PULSEWIDTH 250
#define ESC_INITIALIZE_FRACTION 2 //Send MAX / Fraction to ESC at startup
#define ESC_INITIALIZE_WAIT_TIME 3000 //milliseconds to provide a signal before returning to zero

#define CONTROLLER_TIMEOUT 1000 //How many milliseconds before shutting off motors without a signal from BLE Controller

//Lipo Settings
const uint16_t MIN_MVOLT_PER_CELL = 3300;
const uint16_t NUM_OF_CELLS = 3; 

const uint16_t BATT_READ_FREQ = 100; //Frequency to measure batttery voltage for safety shutdown
const uint8_t BATT_SAMPLE_COUNT = 1; //How many Sample to average of a battery measurement



//Board Specific Settings
#define ESC_1_PIN 4
#define ESC_2_PIN 8

#define DRIVE_MOTOR1_1_PIN 1
#define DRIVE_MOTOR1_2_PIN 3
#define DRIVE_MOTOR2_1_PIN 6
#define DRIVE_MOTOR2_2_PIN 7

#define MODE_BUTTON_PIN 5
#define DEBUG_LED_PIN 10
#define BATT_MEAS_PIN 0

const float BATTERY_MULTIPLIER = 3.7; //Voltage divider: (27k + 10k)/10k

//Naming
#define LEFT 1
#define CENTER 2
#define RIGHT 3

#define FORWARD 1
#define REVERSE 2
#define STOP 3
#define RIGHTSIDE_UP 4
#define UPSIDE_DOWN 5





#endif