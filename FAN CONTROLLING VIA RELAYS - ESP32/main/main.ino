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

#include <IRremote.h>
#include <Arduino.h>
#include <EEPROM.h>

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


#define LOW_SPEED_PIN     25
#define MEDIUM_SPEED_PIN  26
#define HIGH_SPEED_PIN    27
#define SWING_PIN         14
#define IR_PIN            33
#define BUZZER_PIN        32   // safe GPIO for output

// Define button commands
#define CMD_ONOFF     0x03
#define CMD_SPEED     0x07
#define CMD_HICOMFORT 0x0F
#define CMD_TIMER     0x0B
#define CMD_SWING     0x13


uint8_t lastCmd = 0xFF;

unsigned long lastOnOffTime = 0;       // last time we printed
const unsigned long interval = 10000;  // 10 seconds

bool hiComfortActive = false;              // tracks if ON/OFF is “on”



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


// RangeController
std::map<String, int> globalRangeValues;

const int EEPROM_ADDR = 0;  // EEPROM address to store mode byte

// Encode modes as bytes
enum FanMode : uint8_t {
  MODE_LOW = 1,
  MODE_MEDIUM = 2,
  MODE_HIGH = 3
};

#define EEPROM_SIZE 512      // allocate 512 bytes (safe default)
#define EEPROM_ADDR_MODE 0   // address for fan mode
#define EEPROM_ADDR_TOGGLE 1 // address for toggle state

// Save mode to EEPROM
void saveModeToEEPROM(FanMode mode) {
  EEPROM.write(EEPROM_ADDR_MODE, (uint8_t)mode);
  EEPROM.commit(); // actually save to flash
}

// Read mode from EEPROM
FanMode readModeFromEEPROM() {
  uint8_t val = EEPROM.read(EEPROM_ADDR_MODE);
  if (val == 0xFF || val > MODE_HIGH) return MODE_LOW;
  return static_cast<FanMode>(val);
}

// Save toggle state (true/false) to EEPROM
void saveToggleToEEPROM(bool state) {
  EEPROM.write(EEPROM_ADDR_TOGGLE, state ? 1 : 0);
  EEPROM.commit(); // commit changes
}

// Read toggle state from EEPROM
bool readToggleFromEEPROM() {
  uint8_t val = EEPROM.read(EEPROM_ADDR_TOGGLE);
  if (val == 0xFF) return false; // default OFF
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
  delay(500);
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
  tone(BUZZER_PIN, 2500, 100);
  globalPowerState = state;
  if(state){
    ToggleSwing(globalToggleStates["rotation"]);
    ChangeSpeed(globalRangeValues["fanSpeed"]);
  } else {
    if(hiComfortActive){
      hiComfortActive = false;
      ToggleSwing(false);
    }
    turnMotorOff();
    ToggleSwing(false);
  }
  return true;
}

bool changeToggleState(bool state, bool manual){
  if(globalPowerState || !state){
    if(manual){
      updateRotationToggleState(state);
    }
    tone(BUZZER_PIN, 2500, 100);
    globalToggleStates["rotation"] = state;
    ToggleSwing(state);
    return true;
  }
  return false;
}

bool changeRangeValue(int value, bool manual){
  if(manual){
    updateSpeedMode(value);
  }
  hiComfortActive = false;
  tone(BUZZER_PIN, 2500, 100);
  updatePowerState(true);
  
  globalRangeValues["fanSpeed"] = value;
  ChangeSpeed(value);
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
  Serial.printf("[Device: %s]: State for \"%s\" refused to toggle because power is off.\n", deviceId.c_str(), instance.c_str());
  return false;
}

// PowerStateController
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("[Device: %s]: Powerstate changed to %s\r\n", deviceId.c_str(), state ? "on" : "off");
  return changePowerState(state, false); // request handled properly
}


// RangeController
bool onRangeValue(const String &deviceId, const String& instance, int &rangeValue) {
  Serial.printf("[Device: %s]: Value for \"%s\" changed to %d\r\n", deviceId.c_str(), instance.c_str(), rangeValue);
  return changeRangeValue(rangeValue, false);
}

bool onAdjustRangeValue(const String &deviceId, const String& instance, int &valueDelta) {
  int desiredValue = globalRangeValues["fanSpeed"] + valueDelta;
  Serial.printf("[Device: %s]: Value for \"%s\" changed by %d to %d\r\n", deviceId.c_str(), instance.c_str(), valueDelta, desiredValue);
  return changeRangeValue(desiredValue, false);
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

  SinricPro.restoreDeviceStates(true); // Sync cloud states on startup

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
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK); 
  pinMode(BUZZER_PIN, OUTPUT);  // set buzzer as output
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

  // Initialize EEPROM once
  EEPROM.begin(EEPROM_SIZE);


  physicalSetup();
  

  setupWiFi();
  setupSinricPro();

  

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

}

void checkIR(){
  if (IrReceiver.decode()) {
    // Only process if it's **not a repeat signal**
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
      // ignore repeated signal while holding
      IrReceiver.resume();
      return;
    }

    uint8_t cmd = IrReceiver.decodedIRData.command;


    // Trigger action only for first press
    if (cmd != lastCmd) {
      switch (cmd) {
        case CMD_ONOFF:
          Serial.println("ON/OFF button pressed");
          changePowerState(!globalPowerState, true);
          break;

        case CMD_SPEED:  
          Serial.println("SPEED button pressed");
          if(globalPowerState){
            if(globalRangeValues["fanSpeed"] == 1 || globalRangeValues["fanSpeed"] == 2){
              changeRangeValue(globalRangeValues["fanSpeed"] + 1, true);
            } else {
              changeRangeValue(1, true);
            }
          }

          break;

        case CMD_HICOMFORT:
          Serial.println("HI-COMFORT button pressed");
          if(!hiComfortActive){
            for (int i = 0; i < 3; i++) {
            noTone(BUZZER_PIN);       // stop beep
            tone(BUZZER_PIN, 2500);  // 2.5 kHz beep
            delay(100);               //
            noTone(BUZZER_PIN);       // stop beep
            delay(100);               // short pause between beeps

            }
            hiComfortActive = true;
          } else {
            tone(BUZZER_PIN, 2500, 100);
            hiComfortActive = false;
          }
          
          
          
          break;

        case CMD_TIMER:
          tone(BUZZER_PIN, 2500, 100);
          Serial.println("TIMER button pressed");
          break;

        case CMD_SWING:
          Serial.println("SWING button pressed");
          changeToggleState(!globalToggleStates["rotation"], true);
          break;
      }
      lastCmd = cmd;  // remember last pressed button
    }

    IrReceiver.resume(); // ready for next signal
  } else {
    // No IR signal → reset lastCmd so next press registers
    lastCmd = 0xFF;
  }
}

/********
 * Loop *
 ********/

void loop() {
  SinricPro.handle();

  checkIR();

  // Non-blocking 10-second action for ON/OFF
  if (hiComfortActive && millis() - lastOnOffTime >= interval) {
    Serial.println("HI-COMFORT SHOULD RANDOMLY CHANGE NOW.\n"); // your action
    lastOnOffTime = millis(); // reset timer
  }
}