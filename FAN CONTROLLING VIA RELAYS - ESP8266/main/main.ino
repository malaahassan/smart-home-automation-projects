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

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Arduino.h>
#include <EEPROM.h>   // Make sure this is included

// Encode modes as bytes
enum FanMode : uint8_t {
    MODE_LOW    = 1,
    MODE_MEDIUM = 2,
    MODE_HIGH   = 3
};

#ifdef ESP8266
  #include <ESP8266WiFi.h>
#endif
#ifdef ESP32
  #include <WiFi.h>
#endif

#include <SinricPro.h>
#include "Fanwithswingandmemory.h"

#define APP_KEY    "eeb5e18d-4c59-4864-a82d-2a9d8efbd3d9"
#define APP_SECRET "36f96167-877f-463a-8de4-38f094d11a12-17d06084-03af-4a27-9654-dc19ff9a5a4e"
#define DEVICE_ID  "68a1e6c9d06c14a200a5b88a"

#define SSID       "M. Alaa_IoT"
#define PASS       "WIFI PASSWORD REMOVED FOR SECURITY ;)"

#define BAUD_RATE  115200

#define SWING_PIN D0

#define IR_PIN D1
#define BUZZER_PIN D2 
#define LOW_SPEED_PIN D5 
#define MEDIUM_SPEED_PIN D6 
#define HIGH_SPEED_PIN D7 
 


// Updated remote commands
#define CMD_ONOFF     0x807FC03F
#define CMD_SPEED     0x807FE01F
#define CMD_HICOMFORT 0x807FF00F
#define CMD_TIMER     0x807FD02F
#define CMD_SWING     0x807FC837

IRrecv irrecv(IR_PIN);
decode_results results;
uint32_t lastCmd = 0xFFFFFFFF;  // tracks last command
bool hiComfortActive = false;              // tracks if ON/OFF is “on”

void beep(int ms = 100) {
  tone(BUZZER_PIN, 2500);
  delay(ms);
  noTone(BUZZER_PIN);
}

unsigned long lastOnOffTime = 0;       // last time we printed
const unsigned long interval = 10000;  // 10 seconds





Fanwithswingandmemory &fanwithswingandmemory = SinricPro[DEVICE_ID];

/*************
 * Variables *
 ***********************************************
 * Global variables to store the device states *
 ***********************************************/

// ToggleController
std::map<String, bool> globalToggleStates;

// PowerStateController
bool globalPowerState;

bool powerStateFirst = false;


// RangeController
std::map<String, int> globalRangeValues;

#define EEPROM_SIZE 3
const int EEPROM_ADDR = 0; // EEPROM address to store mode byte



// Save mode to EEPROM
void saveModeToEEPROM(FanMode mode) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(EEPROM_ADDR, (uint8_t)mode);
    EEPROM.commit();
    EEPROM.end();
}

// Read mode from EEPROM
FanMode readModeFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    uint8_t val = EEPROM.read(EEPROM_ADDR);
    EEPROM.end();

    if (val > MODE_HIGH) 
        return MODE_LOW; // fallback if invalid
    
    return static_cast<FanMode>(val);
}

const int TOGGLE_EEPROM_ADDR = 1; // Use a different EEPROM address than mode



// Save toggle state (true/false) to EEPROM
void saveToggleToEEPROM(bool state) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(TOGGLE_EEPROM_ADDR, state ? 1 : 0);
    EEPROM.commit();
    EEPROM.end();
}

// Read toggle state from EEPROM
bool readToggleFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    uint8_t val = EEPROM.read(TOGGLE_EEPROM_ADDR);
    EEPROM.end();

    if (val == 0xFF) { // EEPROM not initialized, default OFF
        return false;
    }
    
    return (val == 1);
}

const int POWER_EEPROM_ADDR = 2; // Different address for power state

// Save power state (true = ON, false = OFF)
void savePowerStateToEEPROM(bool state) {
    EEPROM.begin(EEPROM_SIZE); // Allocate at least 3 bytes since we use addresses 0,1,2
    EEPROM.write(POWER_EEPROM_ADDR, state ? 1 : 0);
    EEPROM.commit();
    EEPROM.end();
}

// Read power state from EEPROM
bool readPowerStateFromEEPROM() {
    EEPROM.begin(EEPROM_SIZE);
    uint8_t val = EEPROM.read(POWER_EEPROM_ADDR);
    EEPROM.end();

    if (val == 0xFF) { 
        // EEPROM not initialized, default OFF
        return false;
    }

    return (val == 1);
}

/*UPDATE FUNCTIONS*/


void updateRotationToggleState(bool state) {
  fanwithswingandmemory.sendToggleStateEvent("rotation", state);
  globalToggleStates["rotation"] = state;
  saveToggleToEEPROM(state);
}

// PowerStateController
void updatePowerState(bool state) {
  fanwithswingandmemory.sendPowerStateEvent(state);
  globalPowerState = state;
  savePowerStateToEEPROM(state);
}



// RangeController

void updateSpeedModeEPROM(int value){
  if(value == 1){
    saveModeToEEPROM(MODE_LOW);
  } else if(value == 2){
    saveModeToEEPROM(MODE_MEDIUM);
  } else if(value == 3){
    saveModeToEEPROM(MODE_HIGH);
  }
}

void updateSpeedMode(int value) {
  fanwithswingandmemory.sendRangeValueEvent("fanSpeed", value);
  globalRangeValues["fanSpeed"] = value;
  updateSpeedModeEPROM(value);
  
}





/*PHYSICAL FUNCTIONS*/

void turnMotorOff(){
  digitalWrite(LOW_SPEED_PIN, HIGH);  // Active low (so HIGH means off)
  digitalWrite(MEDIUM_SPEED_PIN, HIGH);  // Active low
  digitalWrite(HIGH_SPEED_PIN, HIGH);  // Active low
}

void turnSwingOff(){
  digitalWrite(SWING_PIN, HIGH);  // Active low
}


void ToggleSwing(bool state) {
  if(globalToggleStates["rotation"] != state){
    updateRotationToggleState(state);
  }
  saveToggleToEEPROM(state);
  digitalWrite(SWING_PIN, state ? LOW : HIGH); 
}

void ChangeSpeed(int value) {
  if(globalRangeValues["fanSpeed"] != value){
    updateSpeedMode(value);
  }
  updateSpeedModeEPROM(value);

  turnMotorOff();
  delay(250);
  if(value == 1){
    digitalWrite(LOW_SPEED_PIN, LOW); 
  } else if(value == 2){
    digitalWrite(MEDIUM_SPEED_PIN, LOW); 
  } else if(value == 3){
    digitalWrite(HIGH_SPEED_PIN, LOW); 
  }
}




/*CHANGE*/
bool changePowerState(bool state, bool manual){
  if(manual){
    updatePowerState(state);
  }
  beep();
  globalPowerState = state;
  savePowerStateToEEPROM(state);

  if(state){
    ChangeSpeed(globalRangeValues["fanSpeed"]);
    ToggleSwing(globalToggleStates["rotation"]);
  } else {
    if(hiComfortActive){
      hiComfortActive = false;
    }
    turnMotorOff();
    turnSwingOff();
    
  }
  return true;
}

bool changeToggleState(bool state, bool manual){
  if(globalPowerState){
    if(manual){
      updateRotationToggleState(state);
    }
    beep();
    globalToggleStates["rotation"] = state;
    ToggleSwing(state);
    
  } else {
    globalToggleStates["rotation"] = state;
    saveToggleToEEPROM(state);
  }
  return true;
}

bool changeRangeValue(int value, bool manual){
  if(globalPowerState){
    if(manual){
      updateSpeedMode(value);
    }
    hiComfortActive = false;
    beep();
    //updatePowerState(true);
    
    globalRangeValues["fanSpeed"] = value;
    ChangeSpeed(value);
  } else {
    globalRangeValues["fanSpeed"] = value;
    updateSpeedModeEPROM(value);
  }
  return true;
}

/*************
 * Callbacks *
 *************/

// ToggleController
bool onToggleState(const String& deviceId, const String& instance, bool &state) {
  if(changeToggleState(state, false)){
    Serial.printf("[Device: %s]: State for \"%s\" set to %s\r\n", deviceId.c_str(), instance.c_str(), state ? "on" : "off");
    return true;
  }
  Serial.printf("[Device: %s]: State for \"%s\" refused to toggle.\r\n", deviceId.c_str(), instance.c_str());
  return false;
}

// PowerStateController
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("[Device: %s]: Powerstate changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
  return changePowerState(state, false); // request handled properly
}


// RangeController
bool onRangeValue(const String &deviceId, const String& instance, int &rangeValue) {
  if(changeRangeValue(rangeValue, false)){
    Serial.printf("[Device: %s]: Value for \"%s\" changed to %d\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
    return true;
  }
  Serial.printf("[Device: %s]: Value for \"%s\" refused to change to %d\r.\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
  return false;
}

bool onAdjustRangeValue(const String &deviceId, const String& instance, int &valueDelta) {
  int desiredValue = globalRangeValues["fanSpeed"] + valueDelta;
  if(desiredValue > 0 && desiredValue <= 3){
    if(changeRangeValue(desiredValue, false)){
      Serial.printf("[Device: %s]: Value for \"%s\" changed by %d to %d\r\n", deviceId.c_str(), instance.c_str(), valueDelta, desiredValue);
      return true;
    }
  }
  Serial.printf("[Device: %s]: Value for \"%s\" refused to change by %d to %d.\r\n", deviceId.c_str(), instance.c_str(), valueDelta, desiredValue);
  return false;
}



/********* 
 * Setup *
 *********/

void setupSinricPro() {

  // ToggleController
  fanwithswingandmemory.onToggleState("rotation", onToggleState);

  // PowerStateController
  fanwithswingandmemory.onPowerState(onPowerState);

  // RangeController
  fanwithswingandmemory.onRangeValue("fanSpeed", onRangeValue);
  fanwithswingandmemory.onAdjustRangeValue("fanSpeed", onAdjustRangeValue);

  SinricPro.onConnected([]{ Serial.printf("[SinricPro]: Connected\r\n"); });
  SinricPro.onDisconnected([]{ Serial.printf("[SinricPro]: Disconnected\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);

  //SinricPro.restoreDeviceStates(true); // Sync cloud states on startup

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

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 10000; // 10 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    Serial.print(".");
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connected");
  } else {
    Serial.println(" failed (timeout)");
    // continue with no WiFi, you can retry later in loop()
  }
}

void physicalSetup(){
  irrecv.enableIRIn();
  
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);       // stop beep

  // Initialize output pins as OUTPUT
  pinMode(LOW_SPEED_PIN, OUTPUT);
  pinMode(MEDIUM_SPEED_PIN, OUTPUT);
  pinMode(HIGH_SPEED_PIN, OUTPUT);
  pinMode(SWING_PIN, OUTPUT);

  // Set all output pins to LOW initially (turn off everything)
  digitalWrite(LOW_SPEED_PIN, HIGH);  // Active low (so HIGH means off)
  digitalWrite(MEDIUM_SPEED_PIN, HIGH);  // Active low
  digitalWrite(HIGH_SPEED_PIN, HIGH);  // Active low
  digitalWrite(SWING_PIN, HIGH);   // Rotary motion off initially
}


void setup() {
  Serial.begin(BAUD_RATE);
  physicalSetup();
  

  setupWiFi();
  setupSinricPro();

  globalPowerState = readPowerStateFromEEPROM();
  Serial.print("Restored power state: ");
  Serial.println(globalPowerState ? "ON" : "OFF");

  FanMode mode = readModeFromEEPROM();

  Serial.print("Restored mode: ");
  switch (mode) {
    case MODE_MEDIUM: 
      globalRangeValues["fanSpeed"] = 2;
      Serial.println("Restored Medium");
      break;
    case MODE_HIGH:   
      globalRangeValues["fanSpeed"] = 3;
      Serial.println("Restored High");
      break;
    default: 
      globalRangeValues["fanSpeed"] = 1;
      Serial.println("Restored Low");
  }

  globalToggleStates["rotation"] = readToggleFromEEPROM();
  Serial.print("Restored toggle state: ");
  Serial.println(globalToggleStates["rotation"] ? "ON" : "OFF");

  
  updateSpeedMode(globalRangeValues["fanSpeed"]);
  updateRotationToggleState(globalToggleStates["rotation"]);
  updatePowerState(globalPowerState);

  if(globalPowerState){
    ChangeSpeed(globalRangeValues["fanSpeed"]);
    ToggleSwing(globalToggleStates["rotation"]);
  } else {
    turnMotorOff();
    turnSwingOff();
  }

  

}

void checkIR(){
  if (irrecv.decode(&results)) {
    uint32_t cmd = results.value;

    // Ignore repeated signals while holding
    if (cmd == lastCmd) {
      irrecv.resume();
      return;
    }

    lastCmd = cmd;

    switch (cmd) {
      case CMD_ONOFF:
        changePowerState(!globalPowerState, true);
        Serial.println("ON/OFF pressed");
        break;

      case CMD_SPEED:
        if(globalPowerState){
            if(globalRangeValues["fanSpeed"] == 1 || globalRangeValues["fanSpeed"] == 2){
              changeRangeValue(globalRangeValues["fanSpeed"] + 1, true);
            } else {
              changeRangeValue(1, true);
            }
          }
        Serial.println("SPEED pressed");
        break;

      case CMD_HICOMFORT:
        if(!hiComfortActive){
            for (int i = 0; i < 3; i++) { beep(); delay(100); }
            hiComfortActive = true;
            if(!globalPowerState){
              updatePowerState(true);
            }
          } else {
            tone(BUZZER_PIN, 2500, 100);
            hiComfortActive = false;
          }
        Serial.println("HI-COMFORT pressed");
        break;

      case CMD_TIMER:
        if(globalPowerState){
          beep();
        }
        Serial.println("TIMER pressed");
        break;

      case CMD_SWING:
        
        if(globalPowerState){
          changeToggleState(!globalToggleStates["rotation"], true);
        }
        Serial.println("SWING pressed");
        break;

      default:
        Serial.print("Unknown command: 0x");
        Serial.println(cmd, HEX);
        break;
    }

    irrecv.resume();  // ready for next signal
  } else {
    // Reset last command if no IR signal
    lastCmd = 0xFFFFFFFF;
  }
}

/********
 * Loop *
 ********/

int randomExcluding(int min, int max, int exclude) {
  int rangeSize = max - min; // size excluding 1 number
  int r = random(rangeSize); // random index in new range
  if (r + min >= exclude) r++; // shift numbers after excluded
  return r + min;
}

void loop() {
  SinricPro.handle();

  checkIR();

  // Non-blocking 10-second action for ON/OFF
  if (hiComfortActive && millis() - lastOnOffTime >= interval) {
    int value = randomExcluding(1, 3, globalRangeValues["fanSpeed"]);
    ChangeSpeed(value);

    Serial.println("HI-COMFORT RANDOMLY CHANGED SPEED TO "); // your action
    Serial.println(value);
    Serial.println("\n");

    lastOnOffTime = millis(); // reset timer
  }
}