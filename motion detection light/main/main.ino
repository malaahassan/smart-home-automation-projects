

const int FIRSTSENSOR_PIN = 2;
const int SECONDSENSOR_PIN = 7;
const int RELAY_PIN = 4;

void setup() {
  Serial.begin(9600);
   Serial.println("Robojax Arduino Tutorial");
   Serial.println("HC-SR501 sensor with relay");
  pinMode(FIRSTSENSOR_PIN, INPUT);// Define SENSOR_PIN as Input from sensor
  pinMode(SECONDSENSOR_PIN, INPUT);// Define SENSOR_PIN as Input from sensor
  pinMode(RELAY_PIN, OUTPUT);// Define RELAY_PIN as OUTPUT for relay
}

void loop() {
  
  int motion1 =digitalRead(FIRSTSENSOR_PIN);
  int motion2 =digitalRead(SECONDSENSOR_PIN);
 
  // if motion is detected in 1 OR 2
  if(motion1 || motion2){
    Serial.println("Motion detected");
    digitalWrite(RELAY_PIN, LOW);// Turn the relay ON
  }else{
     Serial.println("===nothing moves");
     digitalWrite(RELAY_PIN,HIGH);// Turn the relay OFF
  }
  delay(500);
  
}
