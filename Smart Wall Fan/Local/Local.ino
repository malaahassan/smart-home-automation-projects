// Define switch pins (inputs)
#define FAN_SPEED_PIN D5
#define ROTARY_MOTION_PIN D6
#define OUTPUT1_PIN D1
#define OUTPUT2_PIN D2
#define OUTPUT3_PIN D7
#define OUTPUT4_PIN D0

// Variables to track the state of the relays
int fanSpeedState = 0; // 0 = Off, 1 = Speed 1, 2 = Speed 2, 3 = Speed 3
bool rotaryMotionState = false;

unsigned long lastDebounceTime = 0;  // Last time the switch was toggled
unsigned long debounceDelay = 50;    // debounce delay in milliseconds

void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);

  // Initialize switch pins as input with pull-up
  pinMode(FAN_SPEED_PIN, INPUT_PULLUP);
  pinMode(ROTARY_MOTION_PIN, INPUT_PULLUP);

  // Initialize output pins as OUTPUT
  pinMode(OUTPUT1_PIN, OUTPUT);
  pinMode(OUTPUT2_PIN, OUTPUT);
  pinMode(OUTPUT3_PIN, OUTPUT);
  pinMode(OUTPUT4_PIN, OUTPUT);

  // Set all output pins to LOW initially (turn off everything)
  digitalWrite(OUTPUT1_PIN, HIGH);  // Active low (so HIGH means off)
  digitalWrite(OUTPUT2_PIN, HIGH);  // Active low
  digitalWrite(OUTPUT3_PIN, HIGH);  // Active low
  digitalWrite(OUTPUT4_PIN, HIGH);   // Rotary motion off initially
}

void loop() {
  // Read the inputs
  bool fanSpeedStatePressed = digitalRead(FAN_SPEED_PIN) == LOW;
  bool rotaryMotionPressed = digitalRead(ROTARY_MOTION_PIN) == LOW;

  // Handle debounce for fan speed input (cycling speed 1, 2, 3, Off)
  if (fanSpeedStatePressed && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    fanSpeedState = (fanSpeedState + 1) % 4;  // Cycles through 0 (Off), 1, 2, 3 (Speed 1, Speed 2, Speed 3)

    // Turn off all speeds first
    digitalWrite(OUTPUT1_PIN, HIGH);
    digitalWrite(OUTPUT2_PIN, HIGH);
    digitalWrite(OUTPUT3_PIN, HIGH);

    // Turn on the corresponding fan speed relay based on fanSpeedState
    switch (fanSpeedState) {
      case 1:
        digitalWrite(OUTPUT1_PIN, LOW);  // Speed 1
        Serial.println("Speed 1 turned on.");
        break;
      case 2:
        digitalWrite(OUTPUT2_PIN, LOW);  // Speed 2
        Serial.println("Speed 2 turned on.");
        break;
      case 3:
        digitalWrite(OUTPUT3_PIN, LOW);  // Speed 3
        Serial.println("Speed 3 turned on.");
        break;
      case 0:
        // All off
        digitalWrite(OUTPUT4_PIN, HIGH);
        Serial.println("Fan turned off.");
        break;
    }
    delay(200);  // Debounce delay for the fan speed input
  }

  // Handle rotary motion input
  if (rotaryMotionPressed && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();
    if(fanSpeedState != 0){
      rotaryMotionState = !rotaryMotionState;  // Toggle rotary motion state
      if (rotaryMotionState) {
        digitalWrite(OUTPUT4_PIN, HIGH);  // Turn on rotary motion (active high)
        Serial.println("Rotary motion turned on.");
      } else {
        digitalWrite(OUTPUT4_PIN, LOW);  // Turn off rotary motion
        Serial.println("Rotary motion turned off.");
      }
    }
    
    delay(200);  // Debounce delay for the rotary motion input
  }
}
