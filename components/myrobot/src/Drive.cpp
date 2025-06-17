#include <Arduino.h>
#include "Constants.h"
#include "Drive.h"
#include "DriveMotor.h"


Drive::Drive(): leftMotor(DRIVE_MOTOR1_1_PIN, DRIVE_MOTOR1_2_PIN, false), 
rightMotor(DRIVE_MOTOR2_1_PIN, DRIVE_MOTOR2_2_PIN, false){
    setForwardInputLimits();
    setLateralInputLimits();
    //setPwmResolution();
}
//Programming note:  called a member initialization list, and it is used in the constructor 
//of a class to initialize the member variables before the body of the constructor is executed.

void Drive::begin(){
    leftMotor.begin();
    rightMotor.begin();
    maxPwmVal = (1 << DRIVE_MOTOR_PWM_RESOLUTION) - 1;
    Serial.print("Max PWM: ");
    Serial.println(maxPwmVal);
}

/**
 * Sets the minimum and maximum expected input values to set the speed to.
 * Default: -512 to 511
 * @param minInput      Minimum input value from the controller
 * @param maxInput      Maximum input value from the controller
 */
void Drive::setForwardInputLimits(int minInput, int maxInput){
    _minForwardInput = minInput;
    _maxForwardInput = maxInput;
}

/**
 * Sets the minimum and maximum expected input values to set the speed to.
 * Default: -512 to 511
 * @param minInput      Minimum input value from the controller
 * @param maxInput      Maximum input value from the controller
 */
void Drive::setLateralInputLimits(int minInput, int maxInput){
    _minLateralInput = minInput;
    _maxLateralInput = maxInput;
}


/**
 * Sets the number of bits the the PWM resolution will consist of
 * Default: 8 
 * @param resolution        Number of bits in the PWM resolution
 */
/*
void Drive::setPwmResolution(int resolution){
    _resolution = resolution;
    _resolution = constrain(_resolution, 8, 12);
    maxPwmVal = (2^_resolution)-1;
}
*/

/**
 * Sets the speed of both drive motors to provide direction.
 * Call setInputLimits() to define the X and Y input range 
 * 
 * @param joystick_x             Joystick X Input 
 * @param joystick_y             Joystick Y Input 
 * @param orientation   Declare whether the robot is rightside up or upside down
 */
void Drive::combined_direction(int joystick_x, int joystick_y, byte orientation){
    int16_t x_input = joystick_x;
    int16_t y_input = joystick_y;

    int16_t x_input_mapped = map(x_input, _minLateralInput, _maxLateralInput, -maxPwmVal, maxPwmVal);
    int16_t y_input_mapped = map(y_input, _minForwardInput, _maxForwardInput, -maxPwmVal, maxPwmVal);

    int16_t leftMotorSpeed;
    int16_t rightMotorSpeed;

    //If bot is flipped, motors must rotate the opposite direction 
    //to go forward and Right<->Left are swapped now
    if(orientation == RIGHTSIDE_UP){
        leftMotorSpeed = y_input_mapped + x_input_mapped;
        rightMotorSpeed = y_input_mapped - x_input_mapped;
    }
    else if(orientation == UPSIDE_DOWN){
        leftMotorSpeed = y_input_mapped - x_input_mapped;
        rightMotorSpeed = y_input_mapped + x_input_mapped;
    }

    //Convert back to 0 to Max PWM Value range for out drive motor input
    //Set direction based on positive or negative
    int left_speed = constrain(abs(leftMotorSpeed), 0, maxPwmVal);
    int right_speed = constrain(abs(rightMotorSpeed), 0, maxPwmVal);

    if(leftMotorSpeed < 0){leftMotor.setSpeed(left_speed, REVERSE, orientation);}
    else{leftMotor.setSpeed(left_speed, FORWARD, orientation);}

    if(rightMotorSpeed < 0){rightMotor.setSpeed(right_speed, REVERSE, orientation);}
    else{rightMotor.setSpeed(right_speed, FORWARD, orientation);}
}

/**
 * Sets the speed of the motors indiviually using two joysticks, one dedicated for each motor
 * 
 * @param left_input             Input to Control Left Motor
 * @param right_input            Input to Control the Right Motor 
 * @param orientation            Declare whether the robot is rightside up or upside down
 */
void Drive::two_stick_drive(int left_input, int right_input, byte orientation){

    int leftMotorSpeed = map(left_input, _minForwardInput, _maxForwardInput, -maxPwmVal, maxPwmVal);
    int rightMotorSpeed = map(right_input, _minForwardInput, _maxForwardInput, -maxPwmVal, maxPwmVal);


    //If bot is flipped, Left is now right, right is left
    if(orientation == UPSIDE_DOWN){
        int16_t temp = leftMotorSpeed;
        leftMotorSpeed = rightMotorSpeed;
        rightMotorSpeed = temp;
    }

    //Convert back to 0 to Max PWM Value range for out drive motor input
    //Set direction based on positive or negative
    int left_speed = constrain(abs(leftMotorSpeed), 0, maxPwmVal);
    int right_speed = constrain(abs(rightMotorSpeed), 0, maxPwmVal);

    if(leftMotorSpeed < 0){leftMotor.setSpeed(left_speed, REVERSE, orientation);}
    else{leftMotor.setSpeed(left_speed, FORWARD, orientation);}

    if(rightMotorSpeed < 0){rightMotor.setSpeed(right_speed, REVERSE, orientation);}
    else{rightMotor.setSpeed(right_speed, FORWARD, orientation);}

}

//Stop both drive motors
void Drive::stop(){
    leftMotor.setSpeed(0, STOP, RIGHTSIDE_UP);
    rightMotor.setSpeed(0, STOP, RIGHTSIDE_UP);
}

