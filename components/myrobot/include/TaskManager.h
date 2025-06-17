#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <Arduino.h>
#include "Constants.h"
#include "Drum.h"
#include "Drive.h"
#include "PowerFunctions.h"

class TaskManager{
    public:
        TaskManager();
        void begin();
        void update(bool isConnected, int leftDriveInput, int rightDriveInput, int escInput);
        void run();
        void stopAllMotors();
        

    private:
        Drive drive;
        Drum drum;
        PowerFunctions powerFunctions;
        uint32_t _controllerTimeout;
        uint32_t lastUpdateTime;
        bool isStopped;

};

#endif