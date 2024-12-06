#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_NeoPixel.h>
#include <BH1750.h>

#include "ESPAutoWiFiConfig.h"

#define PIN_WS2812B  D4  // Le pin ESP8266 qui se connecte à WS2812B
#define NUM_PIXELS     300  // Le nombre de LEDs (pixels) sur WS2812B
#define WAIT_INTERVAL 5000
#define DELAY_INTERVAL 500
#define LUMI_INTERVAL 700
#define SCL_BH1750 D2
#define SDA_BH1750 D1
#define DETECT_DOWN D6
#define DETECT_UP D7


enum MOVE  {IDLE,UP,DOWN,BOTH,WAIT,OFF,LUMI};
MOVE Move = IDLE;
unsigned long previousMillis = 0; 
int num_pixel_up = 0;
int num_pixel_down = 0;
uint8_t Brigthness = 0;


Adafruit_NeoPixel RubanLed(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
//TwoWire BusI2c() = TwoWire(0);

// put function declarations here:

BH1750 lightMeter;




void ScanI2c()
{
    byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknown error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

}

void IRAM_ATTR Presence_down() {
  bool etat_detecteur_l = digitalRead(DETECT_DOWN);
  if (etat_detecteur_l) {
    previousMillis = millis();    
    Serial.println("Presence down ok");
    if (Move == IDLE) Move = UP;
    else Move = BOTH;
    num_pixel_up = 0;
  }
  else Serial.println("Absence down ok");
  uint16_t lux = lightMeter.readLightLevel();
  Serial.print("Luminosité : ");
  Serial.print(lux);
  Serial.println(" lux");
}

void IRAM_ATTR Presence_up() {
  bool etat_detecteur_l = digitalRead(DETECT_DOWN);
  if (etat_detecteur_l) {
    previousMillis = millis();
    Serial.println("Presence up ok");
    if (Move == IDLE) Move = DOWN;
    else Move = BOTH;
    num_pixel_down = NUM_PIXELS;
  }
  else Serial.println("Absence up ok");
  uint16_t lux = lightMeter.readLightLevel();
  Serial.print("Luminosité : ");
  Serial.print(lux);
  Serial.println(" lux");
}

void clear()
{
  RubanLed.clear();
  RubanLed.show();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("test les escalier");
  pinMode(DETECT_DOWN,INPUT);
  pinMode(DETECT_UP,INPUT);
  
  RubanLed.begin(); // INITIALISER l'objet bande WS2812B (REQUIS)
  clear();

  Wire.begin(D1,D2);
  ScanI2c();
  
  lightMeter.begin();
  attachInterrupt(digitalPinToInterrupt(DETECT_DOWN), Presence_down, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DETECT_UP), Presence_up, FALLING);
  
}



void RampeUP(int pixel)
{
  RubanLed.setPixelColor(pixel, RubanLed.Color(255, 255, 255,50)); // it only takes effect if pixels.show() is called
  RubanLed.show();   // send the updated pixel colors to the WS2812B hardware.
}

void RampeDown(int pixel)
{
  RubanLed.setPixelColor(pixel, RubanLed.Color(255, 255, 255,50)); // it only takes effect if pixels.show() is called
  RubanLed.show();   // send the updated pixel colors to the WS2812B hardware.
}

void RampeBoth(int pixelup,int pixeldown)
{
  RubanLed.setPixelColor(pixelup, RubanLed.Color(255, 255, 255,50)); // it only takes effect if pixels.show() is called
  RubanLed.setPixelColor(pixeldown, RubanLed.Color(255, 255, 255,50)); // it only takes effect if pixels.show() is called 
  RubanLed.show();   // send the updated pixel colors to the WS2812B hardware.

}

void lumidown(uint8_t lumi)
{
  RubanLed.setBrightness(lumi);
  RubanLed.show();
}



void loop() {
  
  if (ESPAutoWiFiConfigLoop()) {  // handle WiFi config webpages
    return;  // skip the rest of the loop until config finished
  }
  unsigned long currentMillis = millis();
  switch (Move) {
    case UP:   if (currentMillis - previousMillis >= DELAY_INTERVAL) {
                  RampeUP(num_pixel_up++);
                  previousMillis = currentMillis;
               }
               if (num_pixel_up >= NUM_PIXELS) Move = WAIT;
               break;
    case DOWN: if (currentMillis - previousMillis >= DELAY_INTERVAL) {
                  RampeDown(num_pixel_down--);
                  previousMillis = currentMillis;
               }        
               if (num_pixel_down <=0) Move = WAIT;
               break;          
    case WAIT: if (currentMillis - previousMillis >= WAIT_INTERVAL)  {
                 previousMillis = currentMillis;
                 Move = LUMI;
                 Brigthness = RubanLed.getBrightness();
               }
               break;
    case BOTH: if (currentMillis - previousMillis >= DELAY_INTERVAL) {
                  RampeBoth(num_pixel_up++,num_pixel_down--);
                  previousMillis = currentMillis;
               }        
               if (num_pixel_down <=0) num_pixel_down= 0;
               if (num_pixel_up >= NUM_PIXELS) num_pixel_up = NUM_PIXELS;
               if ((num_pixel_down <=0) && (num_pixel_up >= NUM_PIXELS)) Move = WAIT;
               break;    
    case LUMI: if (currentMillis - previousMillis >= LUMI_INTERVAL) {
                  lumidown(Brigthness--);
                  previousMillis = currentMillis;
               }        
               if (Brigthness <=0) Move = OFF;
               break;     
    case OFF: clear();Move=IDLE;break;
    case IDLE: break;
  }
  delay(200);
}
