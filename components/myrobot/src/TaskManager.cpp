
#include <TaskManager.h>
#include <Arduino.h>
#include <Constants.h>
#include <Drum.h>
#include <Drive.h>
#include <PowerFunctions.h>
#include "esp_log.h"


static const char* TAG = "TaskManager";

TaskManager::TaskManager()
  : drum(ESC_1_PIN),
    _isConnected(false),
    _leftInput(0),
    _rightInput(0),
    _escInput(0),
    lastUpdateTime(0),
    _controllerTimeout(CONTROLLER_TIMEOUT),
    isStopped(true),
    pendingUpdate(false),
    taskHandle(nullptr)
{}

void TaskManager::begin(){
    // init submodules
    drum.begin();
    drum.setInputLimits(0,1023);

    drive.begin();
    drive.setForwardInputLimits(511,-512);
    drive.setLateralInputLimits(-512,511);

    powerFunctions.begin();

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
                    self->_leftInput,
                    self->_rightInput,
                    RIGHTSIDE_UP
                );
                self->drum.setSpeed(self->_escInput);
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

        // 4) Wait exactly until the next cycle
        vTaskDelayUntil(&lastWake, period);
    }
}



void TaskManager::update(bool isConnected, int leftDriveInput, int rightDriveInput, int escInput){
    // simply stash the latest values
    _isConnected = isConnected;
    _leftInput   = leftDriveInput;
    _rightInput  = rightDriveInput;
    _escInput    = escInput;
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