#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>

#define SLAVE_ADDR 0x08
#define SDA_PIN 21
#define SCL_PIN 22
#define NUM_BYTES 70 
#define LED_PIN 2

// LoRa Pins for VSPI
#define LORA_SCK  14
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_CS   15
#define LORA_RST  27
#define LORA_IRQ  26
#define LORA_FREQ 915E6  

SPIClass SPI_LoRa(VSPI);

// Function to extract value from sensor data string
float extractValue(String data, String key, String unit = "") {
    int keyPos = data.indexOf(key + ":");
    if (keyPos == -1) return 0;
    
    int valueStart = keyPos + key.length() + 1;
    int valueEnd;
    
    if (unit != "") {
        valueEnd = data.indexOf(unit, valueStart);
    } else {
        valueEnd = data.indexOf(",", valueStart);
        if (valueEnd == -1) valueEnd = data.length();
    }
    
    if (valueEnd == -1) return 0;
    
    return data.substring(valueStart, valueEnd).toFloat();
}

unsigned long lastRequestTime = 0;
const unsigned long REQUEST_INTERVAL = 50; // ms between requests

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // ----- I2C Setup -----
    Wire.begin(SDA_PIN, SCL_PIN);
    Serial.println("I2C initialized");
    
    // ----- LoRa Setup -----
    pinMode(LORA_RST, OUTPUT);
    digitalWrite(LORA_RST, LOW);
    delay(10);
    digitalWrite(LORA_RST, HIGH);
    delay(10);
    
    SPI_LoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    LoRa.setSPI(SPI_LoRa);
    LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
    
    if (!LoRa.begin(LORA_FREQ)) {
        Serial.println("LoRa init failed!");
        while (1);
    }
    
    LoRa.setTxPower(13);  
    Serial.println("LoRa initialized successfully");
    Serial.println("Master ready!");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Request data every REQUEST_INTERVAL ms
    if (currentTime - lastRequestTime >= REQUEST_INTERVAL) {
        lastRequestTime = currentTime;

        // Request data from slave
        int bytes = Wire.requestFrom(SLAVE_ADDR, NUM_BYTES);
        String rawData = "";

        while (Wire.available()) {
            char c = Wire.read();
            rawData += c;
        }

        if (rawData.length() > 0 && !rawData.startsWith("Error")) {
            // Extract sensor values
            float co2 = extractValue(rawData, "CO2", "ppm");
            float temp = extractValue(rawData, "Temp", "C");
            float hum = extractValue(rawData, "Hum", "%");
            int ldr = (int)extractValue(rawData, "LDR");
            float soil = extractValue(rawData, "Soil", "%");
            float tds = extractValue(rawData, "TDS", "ppm");

            // Send via LoRa
            LoRa.beginPacket();
            LoRa.print(rawData);
            LoRa.endPacket();

            // Blink LED once to indicate success
            digitalWrite(LED_PIN, HIGH);
            delay(50);
            digitalWrite(LED_PIN, LOW);

            // Optional: print to Serial
            Serial.print("Data sent: ");
            Serial.println(rawData);
        }
    }

}
