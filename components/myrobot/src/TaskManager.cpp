
#include <TaskManager.h>
#include <Arduino.h>
#include <Constants.h>
#include <Drum.h>
#include <Drive.h>
#include <PowerFunctions.h>
#include <Buttons.h>
#include "esp_log.h"
#include "LED.h"
#include "esp_task_wdt.h"



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

    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));

    for (;;) {

        vTaskDelayUntil(&lastWake, period);


        //Battery‐State check
        uint8_t batteryState = self->powerFunctions.getBatteryState();
        if( batteryState == BATTERY_WARN) {
            ESP_LOGD(TAG, "Battery Warning");
        }
        else if (batteryState == BATTERY_LOW && ENABLE_LOW_BATTERY_SHUTDOWN) {

            self->stopAllMotors();
        }

        //If there's a new controller update pending, apply it
        if (self->pendingUpdate) {
            if (self->_isConnected) {
                if( batteryState != BATTERY_LOW ){
                    //Put in things that can be ONLY be updated if the battery is not low
                    //This is the safer section as it protects the battery from overdrain
                    self->drive.two_stick_drive(self->_leftDriveInput, self->_rightDriveInput, RIGHTSIDE_UP);
                    self->drum.setSpeed(self->_forwardEscInput, self->_reverseEscInput);
                    self->motorsStopped = false;
                }
                else{
                    //Put in things that can be updated even if voltage is low
                    //Be careful not to put anything that could draw high current and could overdrain the battery
                }
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

        ESP_ERROR_CHECK(esp_task_wdt_reset());   // feed for THIS task
    }
}


/**
 * Update the task manager with recent controller values
 * @param isConnected         Is the controller actively connected
 * @param ControllerState     Pass the values of the controller inputs
 *    
 */
void TaskManager::update(bool isConnected, const ControllerState& cs){
    _isConnected = isConnected;
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