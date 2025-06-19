#include "WebInterface.h"
#include "Constants.h"
#include <WebServer.h>        // you already have this
#include <Update.h>           // for OTA update routines
#include <Preferences.h>
#include "esp_log.h"

static const char* TAG = "WebInterface";

static const char* AP_SSID = "ESP32-Robot";
static const char* AP_PSK  = "";

WebInterface::WebInterface(WebServer& srv)
  : server(srv)
{}

void WebInterface::begin() {
    WiFi.mode(WIFI_AP);
    if (strlen(AP_PSK)) WiFi.softAP(AP_SSID, AP_PSK);
    else                WiFi.softAP(AP_SSID);
    ESP_LOGI(TAG, "AP IP: %s", WiFi.softAPIP().toString().c_str());

    server.on("/",       [this]{ handleRoot();   });
    server.on("/update", [this]{ handleUpdate(); });
    server.on("/status", [this]{ handleStatus(); });

    server.begin();
}

void WebInterface::handleClient() {
    server.handleClient();
}

void WebInterface::handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html><head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Robot Control</title>
</head><body>
  <h2>CRobot Control</h2>


  <hr>
  <h3>Firmware Update</h3>
  <form action="/upload" method="post" enctype="multipart/form-data">
    <input type="file" name="firmware" accept=".bin"><br><br>
    <input type="submit" value="Upload & Update">
  </form>

  <hr>
  <h3>Status:</h3>
  <div>Current speed: <span id="currentSpeed">–</span> FPS</div>
  <div>ESP Control: <span id="espControl">–</span></div>

  <script>
    function fetchStatus(){
      fetch('/status')
        .then(r=>r.json())
        .then(js=>{
          document.getElementById('currentSpeed').textContent = js.speed;
          document.getElementById('espControl').textContent = js.control;
        })
        .catch(err=>console.log('status err',err));
    }
    setInterval(fetchStatus, 1000);
    fetchStatus();
  </script>

  <hr>
  <footer><small>)rawliteral";

    html += VERSION;

    html += R"rawliteral(</small></footer>
</body></html>
)rawliteral";

    server.send(200, "text/html", html);
}

void WebInterface::handleUpdate() {
}

void WebInterface::handleStatus() {

}