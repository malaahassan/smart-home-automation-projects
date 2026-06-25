
//RFID
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
 
#define SS_PIN 10
#define RST_PIN 9
#define LED_G 4 //define green LED pin
#define LED_R 5 //define red LED
#define BUZZER 7 //buzzer pin
#define BUTTON 2 //BUTTON pin
#define RELAY 8 //RELAY pin

/*const unsigned long eventInterval = 15000;
const unsigned long warningInterval = 5000;
unsigned long previousTime;
unsigned long previousTimeDoor;
bool lightOn = false;
bool isDoorOpen = false;
bool isSirenOn = false*/

int readsuccess;

/* the following are the UIDs of the card which are authorised
    to know the UID of your card/tag use the example code 'DumpInfo'
    from the library mfrc522 it give the UID of the card as well as
    other information in the card on the serial monitor of the arduino*/
byte defcard[][4]={ {0xB9,0xC3,0xBC,0x6E},
{0xB1,0x5B,0x3E,0x20},
{0x2D,0x81,0xDA,0xC3},
{0x5E,0xBF,0xDC,0xC3},
{0xCD,0x31,0xE4,0xC3},
{0xBD,0xCA,0xE4,0xC3}, 
{0x24,0x5C,0xDA,0xC3},
{0x7E,0xA0,0xDA,0xC3},
{0xD2,0x7F,0xDD,0xC3}, 
{0x4D,0x66,0xE4,0xC3},
{0x87,0xE1,0xD9,0xC3},
{0x42,0x4A,0xE4,0xC3},
{0xE0,0x7C,0xDC,0xC3},
{0x31,0x92,0xE4,0xC3},
{0x54,0x37,0xDC,0xC3},
{0x42,0x6D,0xDC,0xC3},
{0x62,0x6B,0xE4,0xC3},
{0xEA,0x87,0xDA,0xC3},
{0x36,0x1E,0xDD,0xC3},
{0x50,0x2E,0xE4,0xC3},
{0x81,0x24,0xDA,0xC3},
{0xF9,0xD4,0xE3,0xC3},
{0x97,0xD7,0xE3,0xC3},
{0x93,0x9A,0xE3,0xC3},
{0x21,0x49,0xE4,0xC3},
{0xC6,0x1D,0xDA,0xC3},
{0xCD,0x22,0xDA,0xC3},
{0x81,0xEB,0xE3,0xC3},
{0xC6,0x6D,0xDA,0xC3},
{0xAE,0x1D,0xDA,0xC3},
{0x25,0x01,0xDA,0xC3},
{0xEA,0x5F,0xDA,0xC3},
{0x3A,0x81,0xE4,0xC3}}; //for multiple cards
int N=33; //change this to the number of cards/tags you will use
byte readcard[4]; //stores the UID of current tag which is read

MFRC522 rc(SS_PIN, RST_PIN);   // Create MFRC522 instance.
LiquidCrystal_I2C lcd(0x27, 16, 2);

long previousMillis = 0;        // will store last time LED was updated
 
// the follow variables is a long because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long interval = 60000;
int lighton = false;

void setup() 
{
  digitalWrite(RELAY,HIGH);// Turn the relay OFF
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  rc.PCD_Init();   // Initiate MFRC522
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RELAY, OUTPUT);// Define RELAY_PIN as OUTPUT for relay
  pinMode(BUTTON, INPUT_PULLUP);
  noTone(BUZZER);
  Serial.println(F("the authorised cards are")); //display authorised cards just to demonstrate you may comment this section out
  for(int i=0;i<N;i++){
    Serial.print(i+1);
    Serial.print("  ");
      for(int j=0;j<4;j++){
        Serial.print(defcard[i][j],HEX);
        }
        Serial.println("");
       }
  Serial.println("");
  Serial.println(F("Scan Access Card to see Details"));
  lcd.begin();
  lcd.print("   Starting");
  lcd.setCursor (0,1); // go to start of 2nd line
  lcd.print("   System...");
  delay(500);
  // Turn off backlight
  lcd.noDisplay();
  lcd.noBacklight();
  // Robojax code for LCD with I2C
}



void OpenDoor() {
  /*isDoorOpen = true;
   previousTimeDoor = currentTime;
   isSirenOn = false;
   */
   unsigned long currentMillis = millis();
  Serial.println("DOOR OPENED");
  digitalWrite(RELAY, LOW);// Turn the relay ON
  digitalWrite(LED_G, HIGH);
  tone(BUZZER, 2500);
  delay(300);
  noTone(BUZZER);
  delay(5000);
  digitalWrite(LED_G, LOW);
  lcd.display();
  lcd.backlight();
  lcd.clear();
  lcd.print("Light on");
  lighton = true;
  previousMillis = currentMillis;
  
}

void loop() 
{
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval && lighton) {
    lighton = false;
    digitalWrite(RELAY,HIGH);// Turn the relay OFF
    lcd.clear();
    lcd.noDisplay();
    lcd.noBacklight();
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_R, LOW);
  }
  if(lighton){
    int numb = (interval - (currentMillis - previousMillis))/1000;
    String message = "Time Left: " + String(numb) + "s";
    for(int n = message.length(); n < 16; n++) // 20 indicates symbols in line. For 2x16 LCD write - 16
    {
      message += " ";
    }
    if(numb <= 0){
      message = "Turning off...";
    }
    if(numb % 2 == 0 && numb <= 10){
      digitalWrite(LED_R, HIGH);
    } else {
      digitalWrite(LED_G, LOW);
      digitalWrite(LED_R, LOW);
    }
    
    Serial.println(message);
    lcd.setCursor (0,1); // go to start of 2nd line
    lcd.print(message);
  }
  /*unsigned long currentTime = millis();
 if(currentTime - previousTime >= eventInterval){
  if(lightOn){
    Serial.println("Light turned Off");
    previousTime = currentTime;
    lightOn = false;
  }
  if(currentTime - previousTimeDoor >= warningInterval){
    if(isDoorOpen){
      isSirenOn = true;
    } else {
      isSirenOn = false;
    }
  }
  if(isDoorOpen && !isSirenOn && (currentTime - previousTimeDoor) % 1000 == 0){
    tone(BUZZER, 2500);
    delay(100);
    noTone(BUZZER);
  }
  
  */
  int pushButton = digitalRead(BUTTON);
  if(pushButton == LOW){
    Serial.println("BUTTON PUSHED");
    lcd.display();
    lcd.backlight();
    lcd.clear();
    lcd.print("Indoor Access");
    OpenDoor();
  }
  
  readsuccess = getid();

if(readsuccess){
  int match = 0;
  int card = 0;
//this is the part where compare the current tag with pre defined tags
  lcd.display();
  lcd.backlight();
  lcd.clear();
  lcd.print("Processing..");
  lcd.setCursor (0,1); // go to start of 2nd line
  lcd.print("Please wait..");
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_R, HIGH);
  for(int i=0;i<N;i++){
    Serial.print("Testing Against Authorised card no: ");
    Serial.println(i+1);
    if(!memcmp(readcard,defcard[i],4)){
      match++;
      card = i;
      break;
      }
  }
  if(match) {
        digitalWrite(LED_R, LOW);
        Serial.println("Authorized access");
        lcd.display();
        lcd.backlight();
        lcd.clear();
        lcd.print("Access Granted");
        lcd.setCursor (0,1); // go to start of 2nd line
        if (card == 0 || card == 1) { lcd.print("Mohamed Alaa"); }
        if (card == 2) { lcd.print("Alaa Ramah"); }
        if (card == 3) { lcd.print("Rasha Abdelmonem"); }
        if (card == 4) { lcd.print("Ayah Alaa"); }
        if (card == 5) { lcd.print("Youssef Alaa"); }
        if (card == 6) { lcd.print("Yahya Alaa"); }
        /*if (card == 7) { lcd.print("Sheikh Kotb"); }
        if (card == 8) { lcd.print("Sheikh Kotb"); }
        if (card == 9) { lcd.print("Abdelrahman"); }
        if (card == 10) { lcd.print("Abdelrahman"); }
        if (card == 11) { lcd.print("2nd Floor"); }
        if (card == 12) { lcd.print("2nd Floor"); }
        if (card == 13) { lcd.print("3rd Floor"); }
        if (card == 14) { lcd.print("3rd Floor"); }
        if (card == 15) { lcd.print("3rd Floor"); }
        if (card == 16) { lcd.print("3rd Floor"); }
        if (card == 17) { lcd.print("4th Floor"); }
        if (card == 18) { lcd.print("4th Floor"); }
        if (card == 19) { lcd.print("4th Floor"); }
        if (card == 20) { lcd.print("4th Floor"); }
        if (card == 21) { lcd.print("Eng. Essam"); }
        if (card == 22) { lcd.print("Eng. Essam"); }
        if (card == 23) { lcd.print("Wael Kenawy"); }
        if (card == 24) { lcd.print("Wael Kenawy"); }
        if (card == 25) { lcd.print("Mohamed Kenawy"); }
        if (card == 26) { lcd.print("Mohamed Kenawy"); }*/
        /*if(content.substring(1) == "B1 5B 3E 20" || content.substring(1) == "B9 C3 BC 6E" || content.substring(1) == "5E BF DC C3" || content.substring(1) == "2D 81 DA C3" || content.substring(1) == "CD 31 E4 C3") { //TO OPEN LIGHT
          previousTime = currentTime;
          lightOn = true;
          Serial.println("Light turned on");
        }*/
        OpenDoor();
      } else {
        digitalWrite(LED_G, LOW);
        Serial.println("Access denied");
        lcd.display();
        lcd.backlight();
        lcd.clear();
        lcd.print("Access Denied");
        lcd.setCursor (0,1); // go to start of 2nd line
        lcd.print("Unathorized Card");
        digitalWrite(LED_R, HIGH);
        tone(BUZZER, 500);
        delay(400);
        noTone(BUZZER);
        delay(400);
        tone(BUZZER, 500);
        delay(400);
        noTone(BUZZER);
        delay(1500);
        lcd.clear();
        lcd.noDisplay();
        lcd.noBacklight();
        digitalWrite(LED_R, LOW);
      }
 
  }
} 

//function to get the UID of the card
int getid(){ 
  if(!rc.PICC_IsNewCardPresent()){
    return 0;
  }
  if(!rc.PICC_ReadCardSerial()){
    return 0;
  }
  Serial.println("THE UID OF THE SCANNED CARD IS:");
 
  for(int i=0;i<4;i++){
    readcard[i]=rc.uid.uidByte[i]; //storing the UID of the tag in readcard
    Serial.print(readcard[i],HEX);
   
  }
   Serial.println("");
   Serial.println("Now Comparing with Authorised cards");
   rc.PICC_HaltA();
 
  return 1;
}
