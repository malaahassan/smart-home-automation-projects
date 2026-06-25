#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SinricPro.h>
#include <SinricProFanUS.h>
#include <SinricProSwitch.h>

// WiFi credentials
const char* ssid = "M. Alaa_IoT";
const char* password = "WIFI PASSWORD REMOVED FOR SECURITY ;)";

// Hardware pins
#define FAN_SPEED_PIN D5
#define ROTARY_MOTION_PIN D6
#define OUTPUT1_PIN D1
#define OUTPUT2_PIN D2
#define OUTPUT3_PIN D7
#define OUTPUT4_PIN D0

// Sinric Pro credentials
#define APP_KEY "eeb5e18d-4c59-4864-a82d-2a9d8efbd3d9"
#define APP_SECRET "36f96167-877f-463a-8de4-38f094d11a12-17d06084-03af-4a27-9654-dc19ff9a5a4e"
#define FAN_ID "6805743d947cbabd200342f7"
#define ROTATION_ID "68055eeb8ed485694c1e7f88"

// System configuration
const unsigned long debounceDelay = 300;
const unsigned long relayDelay = 150;
const unsigned long syncRetryDelay = 2000;
const int maxRetryAttempts = 5;

// System state
struct SyncState {
  bool fanPower = false;
  int fanSpeed = 0;
  bool rotation = false;
  unsigned long lastFanSync = 0;
  unsigned long lastRotationSync = 0;
  int fanSyncAttempts = 0;
  int rotationSyncAttempts = 0;
};

struct DeviceState {
  int fanSpeed = 0; // 0=Off, 1-3=Speeds
  bool rotation = false;
  bool fanNeedsSync = false;
  bool rotationNeedsSync = false;
} currentState, desiredState;

// Timing control
unsigned long lastDebounceTime = 0;
unsigned long lastButtonPress = 0;
SyncState syncState;

void setupWiFi() {
  Serial.printf("\nConnecting to %s", ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
}

void updateRelays() {
  // Turn all speed relays OFF first
  digitalWrite(OUTPUT1_PIN, HIGH);
  digitalWrite(OUTPUT2_PIN, HIGH);
  digitalWrite(OUTPUT3_PIN, HIGH);
  delay(relayDelay);
  
  // Set the correct speed
  if (desiredState.fanSpeed > 0) {
    switch(desiredState.fanSpeed) {
      case 1: digitalWrite(OUTPUT1_PIN, LOW); break;
      case 2: digitalWrite(OUTPUT2_PIN, LOW); break;
      case 3: digitalWrite(OUTPUT3_PIN, LOW); break;
    }
    delay(relayDelay);
  }
  
  // Handle rotation (only if fan is on)
  if (desiredState.fanSpeed > 0) {
    digitalWrite(OUTPUT4_PIN, desiredState.rotation ? LOW : HIGH);
  } else {
    digitalWrite(OUTPUT4_PIN, HIGH);
    desiredState.rotation = false;
    desiredState.rotationNeedsSync = true; // Added this line to force rotation sync when fan turns off
  }
  
  // Update current state after relays are set
  currentState = desiredState;
}

bool syncFanState() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Can't sync - WiFi disconnected");
    return false;
  }

  SinricProFanUS &fan = SinricPro[FAN_ID];
  
  bool powerSuccess = fan.sendPowerStateEvent(currentState.fanSpeed > 0);
  bool speedSuccess = fan.sendRangeValueEvent(currentState.fanSpeed);
  
  if (powerSuccess && speedSuccess) {
    syncState.lastFanSync = millis();
    syncState.fanSyncAttempts = 0;
    syncState.fanPower = currentState.fanSpeed > 0;
    syncState.fanSpeed = currentState.fanSpeed;
    Serial.println("Fan state successfully synced");
    return true;
  }
  
  Serial.println("Fan state sync failed");
  return false;
}

bool syncRotationState() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Can't sync - WiFi disconnected");
    return false;
  }

  SinricProSwitch &rotationSwitch = SinricPro[ROTATION_ID];
  bool success = rotationSwitch.sendPowerStateEvent(currentState.rotation);
  
  if (success) {
    syncState.lastRotationSync = millis();
    syncState.rotationSyncAttempts = 0;
    syncState.rotation = currentState.rotation;
    Serial.println("Rotation state successfully synced");
    return true;
  }
  
  Serial.println("Rotation state sync failed");
  return false;
}

void attemptStateSync() {
  unsigned long now = millis();
  
  // Sync fan state if needed
  if (desiredState.fanNeedsSync) {
    if (syncState.fanSyncAttempts == 0 || 
        (now - syncState.lastFanSync) > syncRetryDelay) {
      
      if (syncFanState()) {
        desiredState.fanNeedsSync = false;
      } else {
        syncState.fanSyncAttempts++;
        if (syncState.fanSyncAttempts >= maxRetryAttempts) {
          desiredState.fanNeedsSync = false;
          Serial.println("Max fan sync attempts reached");
        }
      }
    }
  }
  
  // Sync rotation state if needed
  if (desiredState.rotationNeedsSync) {
    if (syncState.rotationSyncAttempts == 0 || 
        (now - syncState.lastRotationSync) > syncRetryDelay) {
      
      if (syncRotationState()) {
        desiredState.rotationNeedsSync = false;
      } else {
        syncState.rotationSyncAttempts++;
        if (syncState.rotationSyncAttempts >= maxRetryAttempts) {
          desiredState.rotationNeedsSync = false;
          Serial.println("Max rotation sync attempts reached");
        }
      }
    }
  }
}

// Callback handlers
bool onFanPower(const String &deviceId, bool &state) {
  desiredState.fanSpeed = state ? 1 : 0;
  desiredState.rotation = state ? desiredState.rotation : false;
  desiredState.fanNeedsSync = false;
  desiredState.rotationNeedsSync = !state; // Force rotation sync when turning off
  
  updateRelays();
  Serial.printf("Remote control - Fan %s\n", state ? "ON" : "OFF");
  return true;
}

bool onFanSpeed(const String &deviceId, int &speed) {
  desiredState.fanSpeed = speed;
  desiredState.rotation = (speed > 0) ? desiredState.rotation : false;
  desiredState.fanNeedsSync = false;
  desiredState.rotationNeedsSync = (speed == 0); // Force rotation sync when speed set to 0
  
  updateRelays();
  Serial.printf("Remote control - Speed set to %d\n", speed);
  return true;
}

bool onRotationState(const String &deviceId, bool &state) {
  if (desiredState.fanSpeed == 0) return false;
  
  desiredState.rotation = state;
  desiredState.fanNeedsSync = false;
  desiredState.rotationNeedsSync = false;
  
  digitalWrite(OUTPUT4_PIN, desiredState.rotation ? LOW : HIGH);
  currentState.rotation = desiredState.rotation;
  
  Serial.printf("Remote control - Rotation %s\n", state ? "ON" : "OFF");
  return true;
}

void setupSinricPro() {
  SinricProFanUS &fan = SinricPro[FAN_ID];
  fan.onPowerState(onFanPower);
  fan.onRangeValue(onFanSpeed);
  
  SinricProSwitch &rotationSwitch = SinricPro[ROTATION_ID];
  rotationSwitch.onPowerState(onRotationState);
  
  SinricPro.onConnected([](){
    Serial.println("Connected to SinricPro");
    // Force full state sync on connection
    desiredState.fanNeedsSync = true;
    desiredState.rotationNeedsSync = true;
    syncState.fanSyncAttempts = 0;
    syncState.rotationSyncAttempts = 0;
  });
  
  SinricPro.onDisconnected([](){ 
    Serial.println("Disconnected from SinricPro"); 
  });
  
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void handlePhysicalControls() {
  unsigned long now = millis();
  
  // Fan speed button
  if (digitalRead(FAN_SPEED_PIN) == LOW && (now - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = now;
    
    desiredState.fanSpeed = (desiredState.fanSpeed + 1) % 4;
    if (desiredState.fanSpeed == 0) {
      desiredState.rotation = false;
      desiredState.rotationNeedsSync = true; // Added to sync rotation state when fan turns off locally
    }
    
    updateRelays();
    desiredState.fanNeedsSync = true;
    syncState.fanSyncAttempts = 0;
    syncState.rotationSyncAttempts = 0;
    
    Serial.printf("Local control - Speed: %d, Rotation: %s\n", 
                 desiredState.fanSpeed, desiredState.rotation ? "ON" : "OFF");
  }
  
  // Rotation button
  if (digitalRead(ROTARY_MOTION_PIN) == LOW && 
     (now - lastDebounceTime) > debounceDelay && 
     desiredState.fanSpeed != 0) {
    lastDebounceTime = now;
    
    desiredState.rotation = !desiredState.rotation;
    digitalWrite(OUTPUT4_PIN, desiredState.rotation ? LOW : HIGH);
    currentState.rotation = desiredState.rotation;
    
    desiredState.rotationNeedsSync = true;
    syncState.rotationSyncAttempts = 0;
    
    Serial.printf("Local control - Rotation toggled: %s\n", desiredState.rotation ? "ON" : "OFF");
  }
}

void printDebugInfo() {
  static unsigned long lastPrint = 0;
  unsigned long now = millis();
  
  if (now - lastPrint > 5000) { // Print every 5 seconds
    lastPrint = now;
    
    Serial.println("\n=== Current Status ===");
    Serial.printf("Hardware State: Fan Speed %d, Rotation %s\n",
                 currentState.fanSpeed, currentState.rotation ? "ON" : "OFF");
    Serial.printf("Sync State: Fan %s (%d attempts), Rotation %s (%d attempts)\n",
                 desiredState.fanNeedsSync ? "Pending" : "Synced", syncState.fanSyncAttempts,
                 desiredState.rotationNeedsSync ? "Pending" : "Synced", syncState.rotationSyncAttempts);
    Serial.printf("Last Sync: Fan %lu ms ago, Rotation %lu ms ago\n",
                 now - syncState.lastFanSync, now - syncState.lastRotationSync);
    Serial.println("=====================\n");
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize hardware
  pinMode(FAN_SPEED_PIN, INPUT_PULLUP);
  pinMode(ROTARY_MOTION_PIN, INPUT_PULLUP);
  pinMode(OUTPUT1_PIN, OUTPUT);
  pinMode(OUTPUT2_PIN, OUTPUT);
  pinMode(OUTPUT3_PIN, OUTPUT);
  pinMode(OUTPUT4_PIN, OUTPUT);
  
  // Start with all relays OFF
  digitalWrite(OUTPUT1_PIN, HIGH);
  digitalWrite(OUTPUT2_PIN, HIGH);
  digitalWrite(OUTPUT3_PIN, HIGH);
  digitalWrite(OUTPUT4_PIN, HIGH);
  
  setupWiFi();
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
  handlePhysicalControls();
  attemptStateSync();
  printDebugInfo();
}