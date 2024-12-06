#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <NeoPixelBus.h>
#include <Adafruit_NeoPixel.h>
#include <BH1750.h>


#define PIN_WS2812B  D4  // Le pin ESP8266 qui se connecte à WS2812B
#define NUM_PIXELS     300  // Le nombre de LEDs (pixels) sur WS2812B
#define DELAY_INTERVAL 100
#define SCL_BH1750 D2
#define SDA_BH1750 D1
#define DETECT D6

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

void IRAM_ATTR Presence() {
  bool etat_detecteur_l = digitalRead(DETECT);
  if (etat_detecteur_l) Serial.println("Presence ok");
  else Serial.println("Absence ok");
  uint16_t lux = lightMeter.readLightLevel();
  Serial.print("Luminosité : ");
  Serial.print(lux);
  Serial.println(" lux");
}




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("test les escalier");
  pinMode(DETECT,INPUT);
  RubanLed.begin(); // INITIALISER l'objet bande WS2812B (REQUIS)
  RubanLed.setBrightness(25);
  RubanLed.show();
  Wire.begin(D1,D2);
  ScanI2c();
  lightMeter.begin();
  attachInterrupt(digitalPinToInterrupt(DETECT), Presence, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(DETECT), Absence, FALLING);
  
}

void clear()
{
  RubanLed.clear();
  RubanLed.show();
}

void RampeUP()
{
 clear();
 RubanLed.setBrightness(25);
 for (int pixel = 0; pixel < NUM_PIXELS; pixel++) { // for each pixel
    RubanLed.setPixelColor(pixel, RubanLed.Color(255, 255, 255)); // it only takes effect if pixels.show() is called
    RubanLed.show();   // send the updated pixel colors to the WS2812B hardware.

    delay(DELAY_INTERVAL); // pause between each pixel
  }
}

void RampeDown()
{
  Serial.println("rampe down");
 clear(); 
 RubanLed.setBrightness(25);
 for (unsigned int pixel = NUM_PIXELS; pixel > 0 ; pixel--) { // for each pixel
    RubanLed.setPixelColor(pixel, RubanLed.Color(255, 255, 255,50)); // it only takes effect if pixels.show() is called
    RubanLed.show();   // send the updated pixel colors to the WS2812B hardware.

    delay(DELAY_INTERVAL); // pause between each pixel
  }
}

void lumidown()
{
  for (unsigned int i=RubanLed.getBrightness();i>0;i--) {
      RubanLed.setBrightness(i);
      RubanLed.show();
      delay(DELAY_INTERVAL);
  }

}



void loop() {
  // put your main code here, to run 
  clear(); // set all pixel colors to 'off'. It only takes effect if pixels.show() is called

 
  //delay(200);

  delay(200);
/*  RampeUP();
  delay(2000);     // off time
  lumidown();
  RampeDown();
  delay(2000);
  lumidown();
  */
}
