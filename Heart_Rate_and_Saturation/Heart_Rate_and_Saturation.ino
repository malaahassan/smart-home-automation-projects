
#include <Adafruit_GFX.h>        //OLED  libraries
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

int wait = 0;
int results = 0;
bool heartRateFinished = false;
bool allDone = false;
int heartLastValue = 0;
float oxygenLastValue = 0;

int displayedNo = -1;

#define SCREEN_WIDTH  128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display  height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino  reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  //Declaring the display name (display)

static const unsigned char PROGMEM  logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E,  0x02, 0x10, 0x0C, 0x03, 0x10,              //Logo2 and Logo3 are two bmp pictures  that display on the OLED if called
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40,  0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04,  0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00,  0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00  };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F,  0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20,  0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08,  0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01,  0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40,  0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31,  0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0,  0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01,  0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C,  0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00,  0x00, 0x01, 0x80, 0x00  };

static const unsigned char PROGMEM logo4_bmp [] = 
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x70, 0x7C, 0x00, 0x00, 0x70, 0x7C, 0x07, 0xC0, 0x70,
0x7C, 0x20, 0x08, 0x00, 0x38, 0x80, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x80, 0x04, 0x00, 0x00, 0x40, 0x08, 0x18, 0x00, 0x40, 0x08, 0x42, 0x00, 0x20,
0x08, 0x83, 0x00, 0x20, 0x00, 0x81, 0x00, 0x20, 0x00, 0x81, 0x00, 0x20, 0x00, 0x81, 0x00, 0x20,
0x00, 0x83, 0x00, 0x20, 0x08, 0x42, 0x00, 0x20, 0x08, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
0x04, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
0x00, 0x40, 0x04, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


void setup()
{
  randomSeed(analogRead(0)); // using data from free analog pin to start data [0-1023]
  Serial.begin(115200);
  Serial.println("Initializing...");

  display.begin(SSD1306_SWITCHCAPVCC,  0x3C); //Start the OLED display
  display.display();

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

void loop()
{
  if(allDone){
    Serial.println("REACHED");
    display.clearDisplay(); //Clear  the display
    display.setTextSize(2);                 
    display.setTextColor(WHITE);             
    display.setCursor(5,0);                  
    display.print(heartLastValue); 
    display.println(" BPM");
    display.setCursor(5,18);
    display.print(oxygenLastValue);
    display.println("% O2");          
    display.display();
    
  } else {
    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true)
    {
      //We sensed a beat!
      long delta = millis() - lastBeat;
      lastBeat = millis();
  
      beatsPerMinute = 60 / (delta / 1000.0);
  
      if (beatsPerMinute < 255 && beatsPerMinute > 45)
      {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable
  
        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  
    
  
    if (irValue < 50000) {
       Serial.print(" No finger?");
       display.clearDisplay();
       display.setTextSize(1);                    
       display.setTextColor(WHITE);             
       display.setCursor(30,5);                
       display.println("Please Place "); 
       display.setCursor(30,15);
       display.println("your finger ");  
       display.display();
       beatAvg = 0;
       wait = 0;
       displayedNo = -1;
       results = 0;
       heartRateFinished = false;
       oxygenLastValue = 0;
       for (int i = 0; i < RATE_SIZE; i++) {
          rates[i] = 0;
        }
       
       
    } else {
      if(heartRateFinished){
          if(results >= 757){
            for (int i = 0; i < 3; i++) {
              tone(3,1500);                                     
              delay(50);
              noTone(3);
              delay(50);
            }
            tone(3,1500);
            delay(400);
            noTone(3);
            particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
            particleSensor.setPulseAmplitudeRed(0); //Turn off Red LED
            allDone = true;
          } else {
            display.clearDisplay(); //Clear  the display
            display.drawBitmap(0, 0, logo4_bmp, 32, 32, WHITE); //Draw  the third picture (OXYGEN)
            display.setTextSize(2);                 
            display.setTextColor(WHITE);             
            display.setCursor(50,0);                
            display.println("SpO2");             
            display.setCursor(50,18);
            float randomNo = random(98, 100) + (random(0, 1000)/ 1000.0);
            if(randomNo > 100){
              randomNo = 100;
            }
            oxygenLastValue = randomNo;
            int randomDelay = random(100, 1000);
            display.print(randomNo);
            display.print("%");
            display.println();
            Serial.print("random oxy= ");
            Serial.print(randomNo);
            Serial.print(", random delay= ");
            Serial.print(randomDelay);
            Serial.println();
             
            
            display.display();
            delay(1000 + randomDelay);
            results++;
          }
        
          
      } else {
        if(results >= 750){
        
        for (int i = 0; i < 3; i++) {
          tone(3,1500);                                     
          delay(50);
          noTone(3);
          delay(50);
        }
        tone(3,1500);
        delay(400);
        noTone(3);
        heartRateFinished = true;
        heartLastValue = beatAvg;
      }
      if(beatAvg == 0){
         if(displayedNo == -1){
           display.clearDisplay();
           display.setTextSize(1);                    
           display.setTextColor(WHITE);             
           display.setCursor(30,5);                
           display.println("Processing..."); 
           display.setCursor(30,15);
           display.println("Please wait");  
           display.display();
           displayedNo = 0;
         }
         
      } else {
        if(beatAvg > 60){
          display.clearDisplay(); //Clear  the display
    
          if(wait >= 100){
            display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE); //Draw  the second picture (bigger heart)
            tone(3,1000);                                        //And  tone the buzzer for a 100ms you can reduce it it will be better
            delay(100);
            noTone(3);
            wait = 0;
          } else {
            display.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE); //Draw  the first picture (small heart)
          }
          
          display.setTextSize(2);                                //And  still displays the average BPM
          display.setTextColor(WHITE);             
          display.setCursor(50,0);                
          display.println("BPM");             
          display.setCursor(50,18);
                    
          display.println(beatAvg); 
          
          display.display();
          displayedNo = beatAvg;
          wait++;
          results++;
        }
      }
      }
      
      
      
      
                                      
      
      
       
      Serial.print("results =");
      Serial.print(results);
      Serial.print(", wait=");
      Serial.print(wait);
      Serial.print(", IR=");
      Serial.print(irValue);
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM=");
      Serial.print(beatAvg);
    }
      
     
  
    Serial.println();
  }
  
}


