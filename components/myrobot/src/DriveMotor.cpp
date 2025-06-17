
#include <Constants.h>
#include <Arduino.h>
#include <DriveMotor.h>


DriveMotor::DriveMotor(byte fwd_pin, byte rev_pin, bool flip_direction){
    _fwd_pin = fwd_pin;
    _rev_pin = rev_pin;
    _flip_direction = flip_direction;
}

void DriveMotor::begin(){
    Serial.println("Drive Motor Begin");
    if(!ledcAttach(_fwd_pin, DRIVE_MOTOR_PWM_FREQ, DRIVE_MOTOR_PWM_RESOLUTION)){Serial.println("Failed to initialize Drive Motor FWD PWM Pin");}
    if(!ledcAttach(_rev_pin, DRIVE_MOTOR_PWM_FREQ, DRIVE_MOTOR_PWM_RESOLUTION)){Serial.println("Failed to initialize Drive Motor FWD PWM Pin");}
}

/**
 * Sets the speed of an individual drive motor
 * @param speed         Motor Speed between 0-255 
 * @param direction     FORWARD, REVERSE, or STOP
 * @param orientation   Declare whether the robot is rightside up or upside down
 */
void DriveMotor::setSpeed(uint16_t speed, byte direction, byte orientation){

    //Flip motor direction to correct for wiring polarity differences
    if( _flip_direction == true){
        if(direction == FORWARD){direction = REVERSE;}
        else if (direction == REVERSE){direction = FORWARD;}    
    }

    //Flip motor direction to adjust for orientation (CW to CCW)
    //Correction for Left<->Right side swapping is done in Drive.cpp
    if(orientation == UPSIDE_DOWN){
        if(direction == FORWARD){direction = REVERSE;}
        else if (direction == REVERSE){direction = FORWARD;}    
    }


    if(direction == STOP){
        Serial.println("Motor Stopped");
        if(!ledcWrite(_fwd_pin, 0)){Serial.println("Failed to write Motor PWM");}
        if(!ledcWrite(_rev_pin, 0)){Serial.println("Failed to write Motor PWM");}
    }
    else if( direction == FORWARD){
        Serial.print("Forward:");
        Serial.println(speed);
        if(!ledcWrite(_fwd_pin, 255)){Serial.println("Failed to write Motor PWM");}
        if(!ledcWrite(_rev_pin, 255-speed)){Serial.println("Failed to write Motor PWM");}
        //if(!ledcWrite(_fwd_pin, speed)){Serial.println("Failed to write Motor PWM");}
        //if(!ledcWrite(_rev_pin, 0)){Serial.println("Failed to write Motor PWM");}
    }
    else if( direction == REVERSE ){
        Serial.print("Reverse:");
        Serial.println(speed);
        if(!ledcWrite(_fwd_pin, 255-speed)){Serial.println("Failed to write Motor PWM");}
        if(!ledcWrite(_rev_pin, 255)){Serial.println("Failed to write Motor PWM");}
        //if(!ledcWrite(_fwd_pin, 0)){Serial.println("Failed to write Motor PWM");}
        //if(!ledcWrite(_rev_pin, speed)){Serial.println("Failed to write Motor PWM");}
    }

}



