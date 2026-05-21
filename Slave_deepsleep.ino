#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cScd30.h>
#include <esp_sleep.h>

#define LED_PIN 2
#define SLEEP_TIME_US 5 * 1000000ULL  // 5 seconds

// --- Sensor I2C: TwoWire(1) on GPIO 18 (SDA), 19 (SCL) ---
TwoWire SensorWire = TwoWire(1);
SensirionI2cScd30 scd30;

// --- Slave I2C to Master: Default Wire (GPIO 21, 22) ---
#define SLAVE_ADDRESS 0x08

// --- Analog Sensor Pins ---
const int LDR_PIN = 33;
const int SOIL_PIN = 32;
const int TDS_PIN = 35;

// --- Global Data Buffer ---
String sensorData = "";

// --- Slave I2C onRequest handler ---
void onMasterRequest() {
    Wire.write((const uint8_t*)sensorData.c_str(), sensorData.length());
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);

    // --- Sensor I2C Setup (TwoWire 1) ---
    SensorWire.begin(18, 19);
    scd30.begin(SensorWire, SCD30_I2C_ADDR_61);
    scd30.stopPeriodicMeasurement();
    scd30.softReset();
    delay(2000);
    scd30.startPeriodicMeasurement(0);

    // --- Slave I2C Setup (Wire, default GPIO 21/22) ---
    Wire.begin(SLAVE_ADDRESS);  // GPIO 21/22 is default
    Wire.setClock(100000);
    Wire.onRequest(onMasterRequest);

    analogReadResolution(12);
}

void loop() {
    delay(2000);  // Sensor settle time

    // --- Read SCD30 ---
    float co2 = 0.0, temp = 0.0, hum = 0.0;
    if (scd30.blockingReadMeasurementData(co2, temp, hum) != 0) {
        sensorData = "Error reading SCD30!";
        Serial.println(sensorData);
        return;
    }

    // --- LDR ---
    int ldrRaw = analogRead(LDR_PIN);

    // --- Soil ---
    int soilRaw = analogRead(SOIL_PIN);
    float soilPercent = map(soilRaw, 4095, 1500, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);

    // --- TDS ---
    int tdsRaw = analogRead(TDS_PIN);
    float voltage = tdsRaw * (3.3 / 4095.0);
    float tdsValue = (133.42 * voltage * voltage * voltage
                    - 255.86 * voltage * voltage
                    + 857.39 * voltage) * 0.5;

    // --- Compose Sensor Data String ---
    sensorData = "CO2:" + String(co2, 1) + ", "
               + "Temp:" + String(temp, 1) + ", "
               + "Hum:" + String(hum, 1) + ", "
               + "LDR:" + String(ldrRaw) + ", "
               + "Soil:" + String(soilPercent, 1) + ", "
               + "TDS:" + String(tdsValue, 1);

    Serial.println(sensorData);

    // --- Blink LED + Sleep ---
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);

    Serial.println("Waiting for master to read data...");
    delay(1000); 
    Serial.println("Sleeping...");
    esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
    esp_light_sleep_start();
}
