#include <Arduino.h>
#include <FastLED.h>
#include <NeoPixelBus.h>
#include <Adafruit_NeoPixel.h>


#define PIN_WS2812B  D4  // Le pin ESP8266 qui se connecte à WS2812B
#define NUM_PIXELS     300  // Le nombre de LEDs (pixels) sur WS2812B
#define DELAY_INTERVAL 100

Adafruit_NeoPixel RubanLed(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

// put function declarations here:


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("test les escalier");
  RubanLed.begin(); // INITIALISER l'objet bande WS2812B (REQUIS)
  RubanLed.setBrightness(25);
  RubanLed.show();
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

  RampeUP();
  delay(2000);     // off time
  lumidown();
  RampeDown();
  delay(2000);
  lumidown();
}
