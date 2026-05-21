#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd30.h>
#include "esp_sleep.h"

// ---------------- Pins ----------------
#define LED_PIN 2
#define LDR_PIN 33
#define SOIL_PIN 32
#define TDS_PIN 35
#define SLAVE_ADDRESS 0x08

// ---------------- I2C ----------------
TwoWire SensorWire = TwoWire(1);
SensirionI2cScd30 scd30;

// ---------------- Globals ----------------
float co2 = 0, temp = 0, hum = 0;
int ldrRaw = 0;
float soilPercent = 0;
float tdsValue = 0;
String sensorData = "";

// ---------------- Timing ----------------
const uint64_t MAX_CYCLE_MS = 5000;   // Total cycle time cap
const uint64_t MIN_SLEEP_MS = 2000;   // Minimum deep sleep time
uint64_t cycleStart = 0;

// ---------------- FreeRTOS Task Handles ----------------
TaskHandle_t scd30TaskHandle;
TaskHandle_t ldrTaskHandle;
TaskHandle_t soilTaskHandle;
TaskHandle_t tdsTaskHandle;

// ---------------- Task Functions ----------------
void scd30Task(void *param) {
  uint16_t dataReady = 0;
  if (scd30.getDataReady(dataReady) == 0 && dataReady) {
    scd30.blockingReadMeasurementData(co2, temp, hum);
  }
  vTaskDelete(NULL);
}

void ldrTask(void *param) {
  ldrRaw = analogRead(LDR_PIN);
  vTaskDelete(NULL);
}

void soilTask(void *param) {
  int soilRaw = analogRead(SOIL_PIN);
  soilPercent = map(soilRaw, 4095, 1500, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  vTaskDelete(NULL);
}

void tdsTask(void *param) {
  int tdsRaw = analogRead(TDS_PIN);
  float voltage = tdsRaw * (3.3 / 4095.0);
  tdsValue = (133.42 * voltage * voltage * voltage
             - 255.86 * voltage * voltage
             + 857.39 * voltage) * 0.5;
  vTaskDelete(NULL);
}

// ---------------- Master Request Handler ----------------
void onMasterRequest() {
  Wire.write((const uint8_t*)sensorData.c_str(), sensorData.length());
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // I2C Sensor Setup
  SensorWire.begin(18, 19);
  scd30.begin(SensorWire, SCD30_I2C_ADDR_61);
  scd30.stopPeriodicMeasurement();
  scd30.softReset();
  delay(2000);
  scd30.startPeriodicMeasurement(0);

  // I2C Slave Setup
  Wire.begin(SLAVE_ADDRESS);
  Wire.setClock(100000);
  Wire.onRequest(onMasterRequest);

  analogReadResolution(12);

  cycleStart = millis();

  // ---------------- Create tasks for parallel polling ----------------
  xTaskCreatePinnedToCore(scd30Task, "SCD30", 4096, NULL, 1, &scd30TaskHandle, 1);
  xTaskCreatePinnedToCore(ldrTask, "LDR", 2048, NULL, 1, &ldrTaskHandle, 1);
  xTaskCreatePinnedToCore(soilTask, "SOIL", 2048, NULL, 1, &soilTaskHandle, 1);
  xTaskCreatePinnedToCore(tdsTask, "TDS", 2048, NULL, 1, &tdsTaskHandle, 1);
}

// ---------------- Loop ----------------
void loop() {
  // Wait for tasks to finish
  if (eTaskGetState(scd30TaskHandle) == eDeleted &&
      eTaskGetState(ldrTaskHandle) == eDeleted &&
      eTaskGetState(soilTaskHandle) == eDeleted &&
      eTaskGetState(tdsTaskHandle) == eDeleted) {
    
    // Build sensor data string
    sensorData = "CO2:" + String(co2, 1) + ", "
               + "Temp:" + String(temp, 1) + ", "
               + "Hum:" + String(hum, 1) + ", "
               + "LDR:" + String(ldrRaw) + ", "
               + "Soil:" + String(soilPercent, 1) + ", "
               + "TDS:" + String(tdsValue, 1);
    Serial.println(sensorData);

    // Blink LED
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);

    // ---------------- Dynamic deep sleep scheduling ----------------
    uint64_t elapsed = millis() - cycleStart;
    uint64_t sleepTime = (MAX_CYCLE_MS > elapsed) ? (MAX_CYCLE_MS - elapsed) : MIN_SLEEP_MS;
    sleepTime = max(sleepTime, MIN_SLEEP_MS);

    Serial.printf("Cycle: %llu ms, deep sleep for %llu ms...\n", elapsed, sleepTime);

    // Enter deep sleep
    esp_sleep_enable_timer_wakeup(sleepTime * 1000ULL);
    esp_deep_sleep_start();
  }
}
