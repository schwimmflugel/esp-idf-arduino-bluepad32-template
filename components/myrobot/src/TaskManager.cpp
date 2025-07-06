
#include <TaskManager.h>
#include <Arduino.h>
#include <Constants.h>
#include <Drum.h>
#include <Drive.h>
#include <PowerFunctions.h>
#include <Buttons.h>
#include "esp_log.h"
#include "LED.h"


static const char* TAG = "TaskManager";

TaskManager::TaskManager()
  : drum(ESC_1_PIN),
    buttons(MODE_BUTTON_PIN),
    led(DEBUG_LED_PIN),
    _isConnected(false),
    _leftDriveInput(0),
    _rightDriveInput(0),
    _forwardEscInput(0),
    _reverseEscInput(0),
    lastUpdateTime(0),
    _controllerTimeout(CONTROLLER_TIMEOUT),
    motorsStopped(true),
    pendingUpdate(false),
    taskHandle(nullptr)
{}

void TaskManager::begin(){

    drive.begin();
    drive.setForwardInputLimits(511,-512);
    drive.setLateralInputLimits(-512,511);

    drum.begin();
    drum.setInputLimits(0,1023);

    powerFunctions.begin();

    buttons.begin();

    led.begin();

    // create the RTOS task (adjust stack if you overflow)
    xTaskCreatePinnedToCore(
        managerTask,           // function
        "TaskManager",         // name
        4096,                  // stack size in bytes
        this,                  // pvParameters
        tskIDLE_PRIORITY + 1,  // priority
        &taskHandle,           // handle
        APP_CPU_NUM            // core
    );
}

void TaskManager::managerTask(void* pvParameters) {
    auto* self = static_cast<TaskManager*>(pvParameters);
    TickType_t lastWake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(50);  // adjust as needed

    for (;;) {

        vTaskDelayUntil(&lastWake, period);


        //Battery‐low check
        if (self->powerFunctions.isBatteryLow() && ENABLE_LOW_BATTERY_SHUTDOWN) {
            self->stopAllMotors();
            continue; // Skip to the next loop, otherwise the motor will momentarily start before quickly being stopped again
        }

        //If there's a new controller update pending, apply it
        if (self->pendingUpdate) {
            if (self->_isConnected) {
                // drive + drum update
                self->drive.two_stick_drive(self->_leftDriveInput, self->_rightDriveInput, RIGHTSIDE_UP);
                self->drum.setSpeed(self->_forwardEscInput, self->_reverseEscInput);
                self->motorsStopped      = false;
                self->lastUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            }
            else {
                self->stopAllMotors();
            }
            // clear the flag so we don't reapply next loop
            self->pendingUpdate = false;
        }

        //If we've timed out, stop motors (once)
        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - self->lastUpdateTime) >= self->_controllerTimeout){
            self->stopAllMotors();
        }


        ButtonPress buttonVal = self->buttons.checkForPress();
  

        switch(buttonVal){
            case BUTTON_NONE:
            break;

            case BUTTON_SHORT: {
            ESP_LOGI(TAG,"Button: Short Press");
            self->led.enqueuePattern("---", false, 255);
            }
                
            break;

            case BUTTON_LONG:
            ESP_LOGI(TAG,"Button: Long Press");
            self->led.enqueuePattern(".-.", false, 255);                  
            break;
            
            default:
            break;
        }
    }
}


/**
 * Update the task manager with recent controller values
 * @param isConnected         Is the controller actively connected
 * @param ControllerState     Pass the values of the controller inputs
 *    
 */
void TaskManager::update(bool isConnected, const ControllerState& cs){
    _leftDriveInput   = cs.leftStickY;
    _rightDriveInput  = cs.rightStickY;
    _forwardEscInput  = cs.rightTrigger;
    _reverseEscInput  = cs.leftTrigger;
    pendingUpdate     = true;
}

//Stop all motors in the robot. If everything is already stopped, it will pass
void TaskManager::stopAllMotors(){
    if(motorsStopped == false){
        ESP_LOGI(TAG, "Stopping Motors");
        drive.stop();
        drum.stop();
        motorsStopped = true;
    }
}