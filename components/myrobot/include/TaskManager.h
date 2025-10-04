#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <Arduino.h>
#include "Constants.h"
#include "Drum.h"
#include "Drive.h"
#include "PowerFunctions.h"
#include "Buttons.h"
#include "LED.h"
#include "rgbLED.h"
#include "esp_pm.h"

// FreeRTOS
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class TaskManager {
public:
    TaskManager();
    void begin();
    void update(bool isConnected, const ControllerState& cs);
    void stopAllMotors();

private:
    static void managerTask(void* pvParameters);

    Drive drive;
    Drum drum;
    PowerFunctions powerFunctions;
    Buttons buttons;
    LED led;
    rgbLED ledStrip;

    // controller state (written by update(), read by managerTask)
    volatile bool    _isConnected;
    volatile int16_t _leftDriveInput;
    volatile int16_t _rightDriveInput;
    volatile int16_t _forwardEscInput;
    volatile int16_t _reverseEscInput;
    volatile bool pendingUpdate;
    volatile uint32_t lastUpdateTime;

    uint32_t _controllerTimeout;
    volatile bool     motorsStopped;

    TaskHandle_t taskHandle;
};

#endif
