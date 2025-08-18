#include "rgbLED.h"

rgbLED::rgbLED(uint16_t numPixels, uint8_t pin, neoPixelType pixelType)
: strip(numPixels, pin, pixelType),
  count(numPixels),
  pinNum(pin)
{}

void rgbLED::begin() {
  strip.begin();
  strip.clear();
  strip.setBrightness(brightness); // let library handle brightness scaling
  strip.show();
}

void rgbLED::setBrightness(uint8_t b) {
  brightness = b;
  strip.setBrightness(brightness);
  if (curMode == STATIC) {
    strip.fill(strip.Color(lastR, lastG, lastB, lastW));
    strip.show();
  }
}

void rgbLED::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  stopTask();               // ensure rainbow is off
  curMode = STATIC;
  lastR = r; lastG = g; lastB = b; lastW = w;
  strip.setBrightness(brightness);
  strip.fill(strip.Color(r, g, b, w));   // instance Color() respects pixelType
  strip.show();
}

void rgbLED::setRainbow(bool enable, uint16_t speed_ms, uint8_t hue_step) {
  if (enable) {
    speedMs = speed_ms;
    hueStep = hue_step;
    curMode = RAINBOW;
    if (!task) startTask();
  } else {
    curMode = STATIC;
    stopTask();
    // Keep last static color on the strip (no change)
  }
}

void rgbLED::startTask() {
  if (task) return;
  xTaskCreate(taskFn, "rgbLEDTask", 3072, this, 2, &task);
}

void rgbLED::stopTask() {
  if (!task) return;
  vTaskDelete(task);
  task = nullptr;
}

void rgbLED::taskFn(void* arg) {
  auto* self = static_cast<rgbLED*>(arg);

  for (;;) {
    if (self->curMode == RAINBOW) {
      // Draw a simple moving rainbow gradient; gamma-corrected
      for (uint16_t i = 0; i < self->count; ++i) {
        // Hue per pixel: base + i*hueStep (scaled to 0..65535)
        uint16_t hue = self->baseHue + (uint16_t)i * (uint16_t)self->hueStep * 256u;
        uint32_t c   = self->strip.ColorHSV(hue, 255, 255);   // HSV->RGB
        c            = self->strip.gamma32(c);                // gamma fix
        self->strip.setPixelColor(i, c);                      // (W channel remains 0)
      }
      self->strip.setBrightness(self->brightness);
      self->strip.show();

      self->baseHue += 256; // advance ~1/256 of full wheel per frame
      vTaskDelay(pdMS_TO_TICKS(self->speedMs));
    } else {
      // Idle lightly when not animating
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}
