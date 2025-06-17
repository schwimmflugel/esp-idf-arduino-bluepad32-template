
#include <TaskManager.h>
#include <Arduino.h>
#include <Constants.h>
#include <Drum.h>
#include <Drive.h>
#include <PowerFunctions.h>

TaskManager::TaskManager(): drum(ESC_1_PIN){
    _controllerTimeout = CONTROLLER_TIMEOUT;  //Might not be needed because of BLE connection status
    isStopped = true;
}

void TaskManager::begin(){
    drum.begin();
    drum.setInputLimits(0,1023);

    drive.begin();
    drive.setForwardInputLimits(511,-512);
    drive.setLateralInputLimits(-512,511);

    powerFunctions.begin();
}


void TaskManager::update(bool isConnected, int leftDriveInput, int rightDriveInput, int escInput){
    if (isConnected) {
        lastUpdateTime = millis();
        drive.two_stick_drive(leftDriveInput, rightDriveInput, RIGHTSIDE_UP);
        drum.setSpeed(escInput);
        isStopped = false;
    }
    else{
        //stopAllMotors();
    }
}

void TaskManager::run(){
    if(millis() - lastUpdateTime >= _controllerTimeout){
        //stopAllMotors();
    }
    //If power is low, stop all motors
    if(powerFunctions.checkForLowBattery() == true){
        //stopAllMotors();
    }
}

void TaskManager::stopAllMotors(){
    if(isStopped == false){
        Serial.println("Stopping Motors");
        drive.stop();
        drum.stop();
        isStopped = true;
    }
}