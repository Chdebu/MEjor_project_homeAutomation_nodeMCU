#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "AMAN";
const char* password = "aman2007";
const char* url = "https://microserviceastrix-production.up.railway.app/api/device-status";
const char* deviceId = "f4a8bde3-847c-4c92-9b3d-37eaf9b5888e";

const int relayPins[4] = {5, 4, 12, 13};  // Relay pins
const int buttonPins[4] = {14, 15, 3, 1}; // Button pins

bool relayStates[4] = {false, false, false, false};
bool lastButtonStates[4] = {HIGH, HIGH, HIGH, HIGH};

void setup() {
  Serial.begin(115200);
  delay(100);

  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
    digitalWrite(relayPins[i], HIGH); // Active LOW (relay OFF)
  }

  connectWiFi();
}

void loop() {
  handleManualButtons();

  if (WiFi.status() == WL_CONNECTED) {
    fetchAndUpdateRelayStatus(); // Update from cloud
  } else {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }

  delay(10000); // Every 10 seconds
}

void handleManualButtons() {
  for (int i = 0; i < 4; i++) {
    bool currentState = digitalRead(buttonPins[i]);
    if (lastButtonStates[i] == HIGH && currentState == LOW) {
      relayStates[i] = !relayStates[i]; // Toggle
      digitalWrite(relayPins[i], relayStates[i] ? LOW : HIGH);
      Serial.print("Manual Toggle: Relay ");
      Serial.print(i + 1);
      Serial.println(relayStates[i] ? " ON" : " OFF");
      // Optional: Send update to server here if needed
      delay(300); // Debounce
    }
    lastButtonStates[i] = currentState;
  }
}

void connectWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void fetchAndUpdateRelayStatus() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (http.begin(client, url)) {
    http.addHeader("Device-ID", deviceId);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("API Response: " + payload);

      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        for (int i = 0; i < 4; i++) {
          String portKey = "port" + String(i + 1);
          if (doc.containsKey(portKey)) {
            bool apiState = doc[portKey].as<bool>();
            if (relayStates[i] != apiState) {
              relayStates[i] = apiState;
              digitalWrite(relayPins[i], apiState ? LOW : HIGH);
              Serial.print("API Update: Relay ");
              Serial.print(i + 1);
              Serial.println(apiState ? " ON" : " OFF");
            }
          }
        }
      } else {
        Serial.println("JSON parsing failed: " + String(error.c_str()));
      }
    } else {
      Serial.println("HTTP Error: " + String(httpCode));
    }
    http.end();
  } else {
    Serial.println("Failed to begin HTTP connection");
  }
}
