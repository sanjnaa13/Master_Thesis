#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// ---------- WiFi Credentials ----------
const char *ssid = "iPhone";
const char *password = "sanjnaa1312";

// ---------- ThingsBoard MQTT ----------
const char *mqtt_server = "acsd-mimo.etit.tu-chemnitz.de";
const int mqtt_port = 8883;
const char *gateway_token = "venx1cds5buwjy3hw3k3";
const char *gateway_topic = "v1/devices/me/telemetry";

// ---------- LoRa Configuration ----------
#define LORA_SCK   14
#define LORA_MISO  12
#define LORA_MOSI  13
#define LORA_SS    15
#define LORA_RST   27
#define LORA_DI0   26
#define LORA_BAND  915E6

// ---------- Root CA Certificate ----------
const char* root_ca = R"rawliteral(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)rawliteral";

WiFiClientSecure secureClient;
PubSubClient client(secureClient);

// ---------- Setup WiFi ----------
void setup_wifi() {
    Serial.print("Connecting to Wi-Fi");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" connected!");
}

// ---------- Setup MQTT ----------
void setup_mqtt() {
    secureClient.setCACert(root_ca);
    client.setServer(mqtt_server, mqtt_port);
    client.setBufferSize(1024);
    Serial.println("MQTT server configured");
}

void reconnect() {
    while (!client.connected()) {
        String clientId = "gateway-" + String(WiFi.macAddress());
        Serial.print("Connecting to MQTT...");

        if (client.connect(clientId.c_str(), gateway_token, NULL)) {
            Serial.println(" connected!");
        } else {
            Serial.print(" failed, rc=");
            Serial.print(client.state());
            Serial.println(" trying again in 2 seconds");
            delay(2000);
        }
    }
}

// ---------- Setup LoRa ----------
void setup_lora() {
    Serial.println("Initializing LoRa...");

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

    if (!LoRa.begin(LORA_BAND)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    Serial.println("LoRa initialized successfully");
}

// ---------- Helper: Parse "key=value,key2=value2" ----------
void parseKeyValueToJson(const String &data, JsonDocument &doc) {
  int start = 0;
  while (start < data.length()) {
    int colonIndex = data.indexOf(':', start);
    int commaIndex = data.indexOf(',', start);

    if (colonIndex == -1) break;  // no more key:value

    String key = data.substring(start, colonIndex);
    key.trim();

    String value;
    if (commaIndex == -1) {
      value = data.substring(colonIndex + 1);
      start = data.length();
    } else {
      value = data.substring(colonIndex + 1, commaIndex);
      start = commaIndex + 1;
    }
    value.trim();
    value.replace("ppm", "");
    value.replace("%", "");
    value.replace("C", "");
    

    // If numeric -> store as number
    if (value.length() > 0 && isDigit(value.charAt(0)) || value.charAt(0) == '-') {
      doc[key] = value.toFloat();
    } else {
      doc[key] = value;
    }
  }
}

// ---------- Main Setup ----------
void setup() {
    Serial.begin(115200);
    delay(1000);

    setup_wifi();
    setup_mqtt();
    setup_lora();

    Serial.println("Gateway ready!");
}

// ---------- Main Loop ----------
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String incoming = "";
        while (LoRa.available()) {
            incoming += (char)LoRa.read();
        }

        Serial.println("\n--- Received from LoRa ---");
        Serial.println("RSSI: " + String(LoRa.packetRssi()));
        Serial.println("Data: " + incoming);

        // Build JSON payload
        StaticJsonDocument<256> doc;
        parseKeyValueToJson(incoming, doc);
        doc["rssi"] = LoRa.packetRssi();  

        char buffer[256];
        size_t n = serializeJson(doc, buffer);

        // Publish to ThingsBoard
        if (client.publish(gateway_topic, buffer, n)) {
            Serial.println("Successfully published JSON to ThingsBoard");
            Serial.println(buffer);
        } else {
            Serial.println("Failed to publish");
        }
    }

    delay(100);
}