// WebInterface.h
#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include "Constants.h"
#include <Preferences.h>
#include <Update.h>


//extern const float MOTOR_SPEEDS[];
//extern const int   NUM_MOTOR_SPEEDS;
//extern const char  VERSION[];

class WebInterface {
public:
    WebInterface(WebServer& server);
    void begin();
    void handleClient();

private:
    void handleRoot();
    void handleUpdate();
    void handleStatus();

    WebServer& server;
};
