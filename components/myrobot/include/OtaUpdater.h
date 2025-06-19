// OtaUpdater.h
#pragma once

#include <WebServer.h>

class OtaUpdater {
public:
    explicit OtaUpdater(WebServer& server);
    void begin();
private:
    WebServer& server;
    void onUploadFinish();
    void onUploadChunk();
};