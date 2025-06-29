#include "PowerFunctions.h"
#include "Constants.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "PowerFunctions";

PowerFunctions::PowerFunctions()
  : shutdownVoltage_mV(0),
    monitorTaskHandle(nullptr),
    ema_mV(0),
    batteryLow(false),
    samplePeriodTicks(0)
{}

/**
 * @brief  Initialize ADC and start the battery‐monitoring task.
 * 
 * Calculates the shutdown voltage (in mV) based on 
 * MIN_MVOLT_PER_CELL × NUM_OF_CELLS, then spins up 
 * a FreeRTOS task to keep checking battery level.
 */
void PowerFunctions::begin() {
    // ADC setup
    analogReadResolution(12);

    // Compute cutoff
    shutdownVoltage_mV = MIN_MVOLT_PER_CELL * NUM_OF_CELLS;
    ESP_LOGI(TAG, "Shutdown Voltage Set: %d mV", shutdownVoltage_mV);

    // Init EMA to a safe starting point
    ema_mV = shutdownVoltage_mV;

    samplePeriodTicks = pdMS_TO_TICKS(SAMPLE_PERIOD);

    // Spawn monitor task
    xTaskCreatePinnedToCore(
        batteryMonitorTask,
        "BatteryMonitor",
        4096,
        this,
        tskIDLE_PRIORITY + 1,
        &monitorTaskHandle,
        APP_CPU_NUM
    );
}

/**
 * @brief  Query the last‐known “battery low” state.
 * 
 * @return true  if the last measured voltage was at or below shutdownVoltage_mV  
 * @return false if above shutdown voltage
 */
bool PowerFunctions::isBatteryLow() const {
    return batteryLow;
}

// Burst-read or delayed-read ADC as before
float PowerFunctions::readBatteryVoltage() {
    uint32_t rawSum = 0;
    for (uint8_t i = 0; i < BATT_SAMPLE_COUNT; ++i) {
        rawSum += analogRead(BATT_MEAS_PIN);
        vTaskDelay(samplePeriodTicks);
    }
    float avgRaw = float(rawSum) / BATT_SAMPLE_COUNT;
    float volts  = avgRaw * (3.3f / 4095.0f);
    return volts * 1000.0f * BATTERY_MULTIPLIER;
}

/**
 * @brief      RTOS task: periodically samples the battery ADC.
 * @param[in]  pvParameters  A pointer to the PowerFunctions instance (i.e. `this`).
 * 
 * This task will sleep for BATT_READ_FREQ ms between samples.
 * @note       Uses analogReadResolution(12) and BATT_SAMPLE_COUNT samples.
 */
void PowerFunctions::batteryMonitorTask(void* pvParameters) {
    auto* self = static_cast<PowerFunctions*>(pvParameters);
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        // 1) read raw
        float raw_mV = self->readBatteryVoltage();

        // 2) update EMA
        self->ema_mV = EMA_ALPHA * raw_mV
                     + (1.0f - EMA_ALPHA) * self->ema_mV;

        // 3) use filtered value for print & threshold
        ESP_LOGI(TAG, "Filtered Batt V (mV): %.2f", self->ema_mV);

        // 4) hysteresis/debounce logic (simple example)
        static TickType_t lowSince = 0;
        if (self->ema_mV <= self->shutdownVoltage_mV) {
            if (lowSince == 0) lowSince = xTaskGetTickCount();
            // require 3s of low before latch
            if (xTaskGetTickCount() - lowSince >= pdMS_TO_TICKS(3000)) {
                self->batteryLow = true;
                ESP_LOGD(TAG, "LOW BATTERY");
            }
        } else {
            lowSince = 0;
            // only clear after 100mV above shutdown
            if (self->ema_mV >= self->shutdownVoltage_mV + BATT_HYSTERESIS) {
                self->batteryLow = false;
            }
        }

        // 5) wait until next period
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(BATT_READ_FREQ));
    }
}