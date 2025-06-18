#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <Arduino.h>
#include "Constants.h"
#include "Drum.h"
#include "Drive.h"
#include "PowerFunctions.h"

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class TaskManager {
public:
    TaskManager();
    void begin();
    void update(bool isConnected, int leftDriveInput, int rightDriveInput, int escInput);
    void stopAllMotors();

private:
    static void managerTask(void* pvParameters);

    Drive drive;
    Drum drum;
    PowerFunctions powerFunctions;

    // controller state (written by update(), read by managerTask)
    volatile bool    _isConnected;
    volatile int16_t _leftInput;
    volatile int16_t _rightInput;
    volatile int16_t _escInput;
    volatile bool pendingUpdate;
    volatile uint32_t lastUpdateTime;

    uint32_t _controllerTimeout;
    bool     isStopped;

    TaskHandle_t taskHandle;
};

#endif
