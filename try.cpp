#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

// Configuration
const char* ssid = "WHOAMI";
const char* password = "7501534511";
const char* serverUrl = "https://microserviceastrix-production-37f2.up.railway.app/api/device-status";
const char* deviceId = "WRHM2WU21673";

// Pin Configuration
struct Relay {
  uint8_t relayPin;
  uint8_t switchPin;
  bool lastSwitchState;
  unsigned long lastDebounceTime;
  bool switchPressed;
};

Relay relays[4] = {
  {5, 14, HIGH, 0, false},  // Relay 1: D1 (GPIO5), Switch: D5 (GPIO14)
  {4, 12, HIGH, 0, false},  // Relay 2: D2 (GPIO4), Switch: D6 (GPIO12)
  {16, 13, HIGH, 0, false}, // Relay 3: D0 (GPIO16), Switch: D7 (GPIO13)
  {2, 0, HIGH, 0, false}    // Relay 4: D4 (GPIO2), Switch: D3 (GPIO0)
};

bool relayStates[4] = {false, false, false, false};
unsigned long lastServerUpdate = 0;
const unsigned long serverUpdateInterval = 1000; // 1 second update interval

// WiFi and HTTP clients
WiFiClientSecure client;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  
  // Initialize relays and switches
  for (int i = 0; i < 4; i++) {
    pinMode(relays[i].relayPin, OUTPUT);
    pinMode(relays[i].switchPin, INPUT_PULLUP);
    
    // Load saved state from EEPROM
    relayStates[i] = EEPROM.read(i) == 1;
    digitalWrite(relays[i].relayPin, relayStates[i] ? LOW : HIGH);
    
    relays[i].lastSwitchState = digitalRead(relays[i].switchPin);
  }
  
  connectWiFi();
}

void loop() {
  // Handle WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  // Handle physical switches with immediate response
  handleSwitches();
  
  // Handle server updates more frequently
  if (millis() - lastServerUpdate >= serverUpdateInterval) {
    lastServerUpdate = millis();
    checkServerForUpdates();
  }
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  Serial.println("Connecting to WiFi...");
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(100);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed");
  }
}

void handleSwitches() {
  const unsigned long debounceDelay = 30;
  
  for (int i = 0; i < 4; i++) {
    bool currentState = digitalRead(relays[i].switchPin);
    
    // Debounce logic
    if (currentState != relays[i].lastSwitchState) {
      relays[i].lastDebounceTime = millis();
    }
    
    if ((millis() - relays[i].lastDebounceTime) > debounceDelay) {
      // State has changed
      if (currentState != relays[i].lastSwitchState) {
        relays[i].lastSwitchState = currentState;
        
        // Switch pressed (LOW)
        if (currentState == LOW) {
          relays[i].switchPressed = true;
        } 
        // Switch released after being pressed
        else if (relays[i].switchPressed) {
          relays[i].switchPressed = false;
          toggleRelay(i);
        }
      }
    }
  }
}

void toggleRelay(int relayIndex) {
  // Toggle state
  relayStates[relayIndex] = !relayStates[relayIndex];
  
  // Update hardware
  digitalWrite(relays[relayIndex].relayPin, relayStates[relayIndex] ? LOW : HIGH);
  
  // Save to EEPROM
  EEPROM.write(relayIndex, relayStates[relayIndex] ? 1 : 0);
  EEPROM.commit();
  
  // Send to server immediately
  if (WiFi.status() == WL_CONNECTED) {
    sendRelayStateToServer(relayIndex);
  }
  
  Serial.printf("Relay %d manually toggled to %s\n", 
               relayIndex + 1, relayStates[relayIndex] ? "ON" : "OFF");
}

void checkServerForUpdates() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  client.setInsecure();
  client.setTimeout(1500); // 1.5 second timeout
  
  if (http.begin(client, serverUrl)) {
    http.addHeader("Device-ID", deviceId);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(256);
      
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        for (int i = 0; i < 4; i++) {
          String portKey = "port" + String(i + 1);
          if (doc.containsKey(portKey)) {
            bool serverState = doc[portKey].as<bool>();
            if (relayStates[i] != serverState) {
              relayStates[i] = serverState;
              digitalWrite(relays[i].relayPin, serverState ? LOW : HIGH);
              EEPROM.write(i, serverState ? 1 : 0);
              EEPROM.commit();
              Serial.printf("Server updated Relay %d to %s\n", 
                          i + 1, serverState ? "ON" : "OFF");
            }
          }
        }
      }
    }
    http.end();
  }
}

void sendRelayStateToServer(int relayIndex) {
  client.setInsecure();
  client.setTimeout(1500);
  
  String url = String(serverUrl) + "?port" + String(relayIndex + 1) + "=" + 
              (relayStates[relayIndex] ? "1" : "0");
  
  if (http.begin(client, url)) {
    http.addHeader("Device-ID", deviceId);
    http.GET(); // Fire and forget - don't wait for response
    http.end();
  }
}