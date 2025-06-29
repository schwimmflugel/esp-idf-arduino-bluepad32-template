
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
    isStopped(true),
    pendingUpdate(false),
    taskHandle(nullptr)
{}

void TaskManager::begin(){
    // init submodules

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
        // 1) If there's a new controller update pending, apply it
        if (self->pendingUpdate) {
            if (self->_isConnected) {
                // drive + drum update
                self->drive.two_stick_drive(
                    self->_leftDriveInput,
                    self->_rightDriveInput,
                    RIGHTSIDE_UP
                );
                self->drum.setSpeed(self->_forwardEscInput, self->_reverseEscInput);
                self->isStopped      = false;
                self->lastUpdateTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            }
            else {
                self->stopAllMotors();
            }
            // clear the flag so we don't reapply next loop
            self->pendingUpdate = false;
        }

        // 2) If we've timed out, stop motors (once)
        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - self->lastUpdateTime) >= self->_controllerTimeout){
            self->stopAllMotors();
        }

        // 3) Battery‐low check
        if (self->powerFunctions.isBatteryLow() && ENABLE_LOW_BATTERY_SHUTDOWN) {
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

        // 4) Wait exactly until the next cycle
        vTaskDelayUntil(&lastWake, period);
    }
}



void TaskManager::update(bool isConnected, int leftStickInput, int rightStickInput, int rightTriggerInput, int leftTriggerInput){
    // simply stash the latest values
    _isConnected = isConnected;
    _leftDriveInput   = leftStickInput;
    _rightDriveInput  = rightStickInput;
    _forwardEscInput    = rightTriggerInput;
    _reverseEscInput = leftTriggerInput;
    pendingUpdate    = true;
}


void TaskManager::stopAllMotors(){
    if(isStopped == false){
        ESP_LOGI(TAG, "Stopping Motors");
        drive.stop();
        drum.stop();
        isStopped = true;
    }
}