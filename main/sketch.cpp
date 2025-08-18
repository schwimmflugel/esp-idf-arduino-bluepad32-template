// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "sdkconfig.h"

#include <Arduino.h>
#include <Bluepad32.h>

#include "Constants.h"
#include "TaskManager.h"

#include "esp_log.h"
#include "esp_task_wdt.h"

#include "WebInterface.h"
#include <WebServer.h>
#include "OtaUpdater.h"

#include "rgbLED.h"
#include "esp_pm.h"


//
// README FIRST, README FIRST, README FIRST
//
// Bluepad32 has a built-in interactive console.
// By default, it is enabled (hey, this is a great feature!).
// But it is incompatible with Arduino "Serial" class.
//
// Instead of using "Serial" you can use Bluepad32 "Console" class instead.
// It is somewhat similar to Serial but not exactly the same.
//
// Should you want to still use "Serial", you have to disable the Bluepad32's console
// from "sdkconfig.defaults" with:
//    CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n

static const char* TAG = "Main";

WebServer server(80);
WebInterface webIf(server);
OtaUpdater   ota(server);


TaskManager taskManager;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

ControllerState controllerState;

rgbLED ledStrip(4, ESC_2_PIN, NEO_GRBW + NEO_KHZ800);


// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            ESP_LOGI(TAG, "CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            ESP_LOGI(TAG,"Controller model: %s, VID=0x%04x, PID=0x%04x\n", ctl->getModelName(), properties.vendor_id,
                           properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        ESP_LOGI(TAG,"CALLBACK: Controller connected, but could not found empty slot");
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            ESP_LOGI(TAG,"CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController) {
        ESP_LOGI(TAG,"CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl) {
    ESP_LOGD(TAG,
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d",
        ctl->index(),        // Controller Index
        ctl->dpad(),         // D-pad
        ctl->buttons(),      // bitmask of pressed buttons
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->brake(),        // (0 - 1023): brake button
        ctl->throttle(),     // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons(),  // bitmask of pressed "misc" buttons
        ctl->gyroX(),        // Gyro X
        ctl->gyroY(),        // Gyro Y
        ctl->gyroZ(),        // Gyro Z
        ctl->accelX(),       // Accelerometer X
        ctl->accelY(),       // Accelerometer Y
        ctl->accelZ()        // Accelerometer Z
    );
}


void processGamepad(ControllerPtr ctl) {
    // There are different ways to query whether a button is pressed.
    // By query each button individually:
    //  a(), b(), x(), y(), l1(), etc...
    if (ctl->a()) {
        static int colorIdx = 0;
        // Some gamepads like DS4 and DualSense support changing the color LED.
        // It is possible to change it by calling:
        switch (colorIdx % 3) {
            case 0:
                // Red
                ctl->setColorLED(255, 0, 0);
                break;
            case 1:
                // Green
                ctl->setColorLED(0, 255, 0);
                break;
            case 2:
                // Blue
                ctl->setColorLED(0, 0, 255);
                break;
        }
        colorIdx++;
    }

    if (ctl->b()) {
        // Turn on the 4 LED. Each bit represents one LED.
        static int led = 0;
        led++;
        // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
        // support changing the "Player LEDs": those 4 LEDs that usually indicate
        // the "gamepad seat".
        // It is possible to change them by calling:
        ctl->setPlayerLEDs(led & 0x0f);
    }

    if (ctl->x()) {
        // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
        // It is possible to set it by calling:
        // Some controllers have two motors: "strong motor", "weak motor".
        // It is possible to control them independently.
        ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                            0x40 /* strongMagnitude */);
    }

    // Another way to query controller data is by getting the buttons() function.
    // See how the different "dump*" functions dump the Controller info.
    dumpGamepad(ctl);

    // See ArduinoController.h for all the available functions.
}

void processControllers() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                processGamepad(myController);
            }
            else {
                ESP_LOGI(TAG,"Unsupported controller\n");
            }
        }
    }
}


void setup() {

    esp_log_level_set("*", ESP_LOG_DEBUG);

    ESP_LOGI(TAG,"Firmware: %s", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    ESP_LOGI(TAG,"BD Addr: %2X:%2X:%2X:%2X:%2X:%2X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    taskManager.begin(); //Begin first so motor drive pins are set correctly and aren't driving for a few seconds during bootup 

    // Setup the Bluepad32 callbacks, and the default behavior for scanning or not.
    // By default, if the "startScanning" parameter is not passed, it will do the "start scanning".
    // Notice that "Start scanning" will try to auto-connect to devices that are compatible with Bluepad32.
    // E.g: if a Gamepad, keyboard or mouse are detected, it will try to auto connect to them.
    bool startScanning = true;
    BP32.setup(&onConnectedController, &onDisconnectedController, startScanning);

    // Notice that scanning can be stopped / started at any time by calling:
    // BP32.enableNewBluetoothConnections(enabled);

    // "forgetBluetoothKeys()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "forgetBluetoothKeys" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    // BP32.forgetBluetoothKeys();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
    BP32.enableVirtualDevice(false);

    // Enables the BLE Service in Bluepad32.
    // This service allows clients, like a mobile app, to setup and see the state of Bluepad32.
    // By default, it is disabled.
    BP32.enableBLEService(false);

    // 1) De-init any auto-subscribed idle WDT (no error check needed)
    esp_task_wdt_deinit();

    // 2) Configure the Task WDT to ignore idle task on core 0
    esp_task_wdt_config_t wdt_conf = {
        .timeout_ms     = 3000,   // 3 s
        .idle_core_mask = 0,      // do NOT watch IDLE
        .trigger_panic  = true    // reboot on timeout
    };
    ESP_ERROR_CHECK( esp_task_wdt_init(&wdt_conf) );

    // 3) Watch the main Arduino loop task
    ESP_ERROR_CHECK( esp_task_wdt_add(NULL) );


    // Disable Wi-Fi power save and light sleep so RMT can init
    WiFi.setSleep(false);

    ledStrip.begin();
    ledStrip.setBrightness(255);

    // Option 1: solid Red
    //ledStrip.setColor(255, 0, 0, 0);
    ledStrip.setRainbow(true, 20, 25);

    webIf.begin();
    ota.begin();
}

void loop() {
    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    if (dataUpdated){
        processControllers();

        if (myControllers[0] && myControllers[0]->isConnected() && myControllers[0]->hasData()){
            controllerState.leftStickY = myControllers[0]->axisY();
            controllerState.rightStickY = myControllers[0]->axisRY();
            controllerState.rightTrigger = myControllers[0]->throttle();
            controllerState.leftTrigger = myControllers[0]->brake();
            taskManager.update(true, controllerState);
        }
        else{
            controllerState.leftStickY = 0;
            controllerState.rightStickY = 0;
            controllerState.rightTrigger = 0;
            controllerState.leftTrigger = 0;
            taskManager.update(false, controllerState);
        }

    }

    esp_task_wdt_reset();            // feed the WDT

    webIf.handleClient();

    vTaskDelay(pdMS_TO_TICKS(100)); //Can't sample too fast or wifi doesn't work
}
