/*
 * Example
 *
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 * - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 */

 // Custom devices requires SinricPro ESP8266/ESP32 SDK 2.9.6 or later

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_ESP_PORT Serial
  #define NODEBUG_WEBSOCKETS
  #define NDEBUG
#endif

#include <Arduino.h>
#include <EEPROM.h>

#ifdef ESP8266
  #include <ESP8266WiFi.h>
#endif
#ifdef ESP32
  #include <WiFi.h>
#endif

#include <SinricPro.h>
#include "Fanwithswing.h"

#define APP_KEY    "eeb5e18d-4c59-4864-a82d-2a9d8efbd3d9"
#define APP_SECRET "36f96167-877f-463a-8de4-38f094d11a12-17d06084-03af-4a27-9654-dc19ff9a5a4e"
#define DEVICE_ID  "68972c40edeca866fe9b76fb"

#define SSID       "M. Alaa_IoT"
#define PASS       "WIFI PASSWORD REMOVED FOR SECURITY ;)"

#define BAUD_RATE  115200


#define LOW_SPEED_PIN D1
#define MEDIUM_SPEED_PIN D2
#define HIGH_SPEED_PIN D5
#define SWING_PIN D6

#define CHANGE_MODE_PIN D7
#define TOGGLE_SWING_PIN D0

Fanwithswing &fanwithswing = SinricPro[DEVICE_ID];

/*************
 * Variables *
 ***********************************************
 * Global variables to store the device states *
 ***********************************************/

// ToggleController
std::map<String, bool> globalToggleStates;

// PowerStateController
bool globalPowerState;

// ModeController
std::map<String, String> globalModes;


/*UPDATE FUNCTIONS*/

void checkPhysicalFanStatus(){
    int LOW_SPEED_STATUS = (digitalRead(LOW_SPEED_PIN) == LOW);
    int MEDIUM_SPEED_STATUS = (digitalRead(MEDIUM_SPEED_PIN) == LOW);
    int HIGH_SPEED_STATUS = (digitalRead(HIGH_SPEED_PIN) == LOW);
    int SWING_STATUS = (digitalRead(SWING_PIN) == LOW);

    if(LOW_SPEED_STATUS){
      //Serial.printf("LOW\n");
      if(globalPowerState != true){
        updatePowerState(true);
        Serial.printf("Updated power to ON\n");
      }
      if(globalModes["fanSpeed"] != "Low"){
        updateSpeedMode("Low");
        Serial.printf("Updated speed to low\n");
      }
      
    } else if(MEDIUM_SPEED_STATUS){
      //Serial.printf("MEDIUM\n");
      if(globalPowerState != true){
        updatePowerState(true);
        Serial.printf("Updated power to ON\n");
      }
      if(globalModes["fanSpeed"] != "Medium"){
        updateSpeedMode("Medium");
        Serial.printf("Updated speed to Medium\n");
      }
    } else if(HIGH_SPEED_STATUS){
      //Serial.printf("HIGH\n");
      if(globalPowerState != true){
        updatePowerState(true);
        Serial.printf("Updated power to ON\n");
      } else {
        if(globalModes["fanSpeed"] != "High"){
          updateSpeedMode("High");
          Serial.printf("Updated speed to High\n");
        }
      }
      
    } else {
      //Serial.printf("OFF\n");
      if(globalPowerState != false){
        updatePowerState(false);
        Serial.printf("Updated power to OFF\n");
      }
      if(globalModes["fanSpeed"] != "OFF"){
        updateSpeedMode("OFF");
        Serial.printf("Updated speed to OFF\n");
      }

    }

    if(SWING_STATUS){
      if(globalToggleStates["rotation"] != true){
        updateRotationToggleState(true);
        Serial.printf("Updated swing to On\n");
      }
      
    } else {

      if(globalToggleStates["rotation"] != false){
        updateRotationToggleState(false);
        Serial.printf("Updated swing to Off\n");
      }
      
    }
    delay(500);
}

void updateRotationToggleState(bool state) {
  fanwithswing.sendToggleStateEvent("rotation", state);
  globalToggleStates["rotation"] = state;
}

// PowerStateController
void updatePowerState(bool state) {
  fanwithswing.sendPowerStateEvent(state);
  globalPowerState = state;
}



// ModeController

void updateSpeedMode(String mode) {
  fanwithswing.sendModeEvent("fanSpeed", mode, "PHYSICAL_INTERACTION");
  globalModes["fanSpeed"] = mode;
}





/*PHYSICAL FUNCTIONS*/

void ToggleSwing() {
  digitalWrite(TOGGLE_SWING_PIN, HIGH);
  delay(200);
  digitalWrite(TOGGLE_SWING_PIN, LOW);
  delay(500);
  checkPhysicalFanStatus();
}

void ChangeMode() {
  digitalWrite(CHANGE_MODE_PIN, HIGH);
  delay(200);
  digitalWrite(CHANGE_MODE_PIN, LOW);
  delay(1000);
  checkPhysicalFanStatus();
}






/*************
 * Callbacks *
 *************/

// ToggleController
bool onToggleState(const String& deviceId, const String& instance, bool &state) {
  if(globalPowerState){
    Serial.printf("[Device: %s]: State for \"%s\" set to %s\r\n", deviceId.c_str(), instance.c_str(), state ? "on" : "off");
    while(globalToggleStates["rotation"] != state){
      ToggleSwing();
    }
    return true;
  }
  Serial.printf("[Device: %s]: State for \"%s\" refused to toggle because power is off.\n", deviceId.c_str(), instance.c_str());
  return false;
}

// PowerStateController
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("[Device: %s]: Powerstate changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
  if(globalPowerState != state){
    if(state){
      while(globalModes["fanSpeed"] != "Low"){
        ChangeMode();
      }
    } else {
      while(globalModes["fanSpeed"] != "OFF"){
        ChangeMode();
      }
    }
  } else {
    updatePowerState(globalPowerState);
    Serial.printf("Refused to power because power state is not synced with cloud\n");
    return false;
  }
  
  return true; // request handled properly
}

// ModeController
bool onSetMode(const String& deviceId, const String& instance, String &mode) {

  Serial.printf("[Device: %s]: Modesetting for \"%s\" set to mode %s\r\n", deviceId.c_str(), instance.c_str(), mode.c_str());

  while(globalModes["fanSpeed"] != mode){
    ChangeMode();
  }
    
  return true;
}



/********* 
 * Setup *
 *********/

void setupSinricPro() {

  // ToggleController
  fanwithswing.onToggleState("rotation", onToggleState);

  // PowerStateController
  fanwithswing.onPowerState(onPowerState);

  // ModeController
  fanwithswing.onSetMode("fanSpeed", onSetMode);

  SinricPro.onConnected([]{ Serial.printf("[SinricPro]: Connected\r\n"); });
  SinricPro.onDisconnected([]{ Serial.printf("[SinricPro]: Disconnected\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);

};

void setupWiFi() {
  #if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP); 
    WiFi.setAutoReconnect(true);
  #elif defined(ESP32)
    WiFi.setSleep(false); 
    WiFi.setAutoReconnect(true);
  #endif

  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  Serial.printf("connected\r\n");
}

void physicalSetup(){
  pinMode(LOW_SPEED_PIN, INPUT_PULLUP);
  pinMode(MEDIUM_SPEED_PIN, INPUT_PULLUP);
  pinMode(HIGH_SPEED_PIN, INPUT_PULLUP);
  pinMode(SWING_PIN, INPUT_PULLUP);

  pinMode(CHANGE_MODE_PIN, OUTPUT);
  digitalWrite(D0, LOW);

  pinMode(TOGGLE_SWING_PIN, OUTPUT);
  digitalWrite(D7, LOW);

}


void setup() {
  Serial.begin(BAUD_RATE);
  physicalSetup();
  setupWiFi();
  setupSinricPro();

  updatePowerState(false);
  updateSpeedMode("OFF");
  updateRotationToggleState(false);


}

/********
 * Loop *
 ********/

void loop() {
    SinricPro.handle();
    checkPhysicalFanStatus();

}







