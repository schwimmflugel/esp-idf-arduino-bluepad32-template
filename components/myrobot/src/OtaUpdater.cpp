// OtaUpdater.cpp
#
// OtaUpdater.cpp
#include "OtaUpdater.h"
#include <Update.h>
#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "OtaUpdater";

OtaUpdater::OtaUpdater(WebServer& srv)
  : server(srv)
{}

void OtaUpdater::begin() {
    server.on(
      "/upload",
      HTTP_POST,
      std::bind(&OtaUpdater::onUploadFinish, this),
      std::bind(&OtaUpdater::onUploadChunk,  this)
    );
}

void OtaUpdater::onUploadChunk() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        ESP_LOGI(TAG, "OTA: %s", upload.filename.c_str());
        if (upload.currentSize >= 1 && upload.buf[0] != (uint8_t)0xE9) {
            server.send(400, "text/plain", "Not a valid ESP32 .bin!");
            return;
        }
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!Update.end(true)) {
            Update.printError(Serial);
        }
    }
}

void OtaUpdater::onUploadFinish() {
    if (Update.hasError()) {
        server.send(500, "text/plain", "Update failed");
    } else {
        server.send(200, "text/plain", "Update success; rebooting...");
        delay(1000);
        ESP.restart();
    }
}

