#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include <Adafruit_NeoPixel.h>
#include <BH1750.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "LittleFS.h"

#define PIN_WS2812B  D4  // Le pin ESP8266 qui se connecte à WS2812B
#define NUM_PIXELS     300  // Le nombre de LEDs (pixels) sur WS2812B
#define WAIT_INTERVAL 2000
#define DELAY_INTERVAL 200
#define LUMI_INTERVAL 200
#define SCL_BH1750 D2
#define SDA_BH1750 D1
#define DETECT_DOWN D6
#define DETECT_UP D7

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
//IPAddress localGateway;
IPAddress localGateway(192, 168, 1, 254); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Set LED GPIO
const int ledPin = 2;
// Stores LED state

String ledState;
boolean restart = false;







enum MOVE  {IDLE,UP,DOWN,BOTH,WAIT,OFF,LUMI};
MOVE Move = IDLE;
MOVE newMove = IDLE;
unsigned long previousMillis = 0; 
int num_pixel_up = 0;
int num_pixel_down = 0;
uint8_t Brigthness = 0;


Adafruit_NeoPixel RubanLed(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
//TwoWire BusI2c() = TwoWire(0);

// put function declarations here:

BH1750 lightMeter;



// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
    Serial.println("LittleFS mounted successfully");
  }
}

// Read File from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  Serial.println(fileContent);
  return fileContent;
}

// Write file to LittleFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
  file.close();
}

// Initialize WiFi
bool initWiFi() {
  Serial.println("InitWIFI");
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }
  Serial.print("init Wifi ");Serial.println(ip.c_str());
  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());

  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.println("Connecting to WiFi...");
  delay(20000);
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");
    return false;
  }

  Serial.println(WiFi.localIP());
  return true;
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  if(var == "STATE") {
    if(!digitalRead(ledPin)) {
      ledState = "ON";
    }
    else {
      ledState = "OFF";
    }
    return ledState;
  }
  return String();
}

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
    if ((Move == IDLE) | (Move == UP)) newMove = UP;
    else if (Move== DOWN) newMove = BOTH;
    if ((Move == UP) || (Move == BOTH)) ;
    else num_pixel_up = 0;
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
  Serial.begin(115200);
  Serial.println("test les escalier");
  
  initFS();
  // Set GPIO 2 as an OUTPUT
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  ip = readFile(LittleFS, ipPath);
  Serial.println(ip);
  //gateway = readFile (LittleFS, gatewayPath);
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);
  if(initWiFi()) {
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    
    server.serveStatic("/", LittleFS, "/");
    
    // Route to set GPIO state to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, LOW);
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    // Route to set GPIO state to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
      digitalWrite(ledPin, HIGH);
      request->send(LittleFS, "/index.html", "text/html", false, processor);
    });
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("ESP-WIFI-MANAGER", NULL);
    //WiFi.softAPConfig (localIP, localGateway, subnet);
    IPAddress IP = WiFi.softAPIP();
    //IPAddress IP(192,168,1,200);
    Serial.print("AP IP address: ");
    Serial.println(IP); 

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", LittleFS, "/");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        const AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            writeFile(LittleFS, ipPath, ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            writeFile(LittleFS, gatewayPath, gateway.c_str());
          }
          Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      restart = true;
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(1000);
    });
    server.begin();
  } 

  pinMode(DETECT_DOWN,INPUT);
  pinMode(DETECT_UP,INPUT);
  
  RubanLed.begin(); // INITIALISER l'objet bande WS2812B (REQUIS)
  RubanLed.setBrightness(25);
  clear();

  Wire.begin(D1,D2);
  ScanI2c();
  
  lightMeter.begin();
/*  setESPAutoWiFiConfigDebugOut(Serial);
  if (ESPAutoWiFiConfigSetup(D0, true, 0)) { 
    return; // in config mode so skip rest of setup
  }
*/  Serial.println("Setup done");
  attachInterrupt(digitalPinToInterrupt(DETECT_DOWN), Presence_down, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(DETECT_UP), Presence_up, CHANGE);
  
}



void RampeUP(int pixel)
{
  RubanLed.setBrightness(25);
  RubanLed.setPixelColor(pixel, RubanLed.Color(255, 255, 255,50)); // it only takes effect if pixels.show() is called
  RubanLed.show();   // send the updated pixel colors to the WS2812B hardware.
}

void RampeDown(int pixel)
{
  RubanLed.setBrightness(25);
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
  /*
  if (ESPAutoWiFiConfigLoop()) {  // handle WiFi config webpages
    return;  // skip the rest of the loop until config finished
  }
  */
   if (restart){
    delay(5000);
    ESP.restart();
  }
  unsigned long currentMillis = millis();
  switch (Move) {
    case UP:   if (currentMillis - previousMillis >= DELAY_INTERVAL) {
                  RampeUP(num_pixel_up++);
                  previousMillis = currentMillis;
               }
               if (num_pixel_up >= NUM_PIXELS) newMove = WAIT;
               Serial.println("Move : UP");Serial.println(num_pixel_up);
               break;
    case DOWN: if (currentMillis - previousMillis >= DELAY_INTERVAL) {
                  RampeDown(num_pixel_down--);
                  previousMillis = currentMillis;
               }        
               if (num_pixel_down <=0) newMove = WAIT;
               Serial.println("Move : DOWN");
               break;          
    case WAIT: if (currentMillis - previousMillis >= WAIT_INTERVAL)  {
                 previousMillis = currentMillis;
                 newMove = LUMI;
                 Brigthness = RubanLed.getBrightness();
                 Serial.println(Brigthness);
               }
               Serial.println("Move : WAIT");
               break;
    case BOTH: if (currentMillis - previousMillis >= DELAY_INTERVAL) {
                  RampeBoth(num_pixel_up++,num_pixel_down--);
                  previousMillis = currentMillis;
               }        
               if (num_pixel_down <=0) num_pixel_down= 0;
               if (num_pixel_up >= NUM_PIXELS) num_pixel_up = NUM_PIXELS;
               if ((num_pixel_down <=0) && (num_pixel_up >= NUM_PIXELS)) newMove = WAIT;
               Serial.println("Move : BOTH");
               break;    
    case LUMI: if (currentMillis - previousMillis >= LUMI_INTERVAL) {
                  lumidown(Brigthness--);
                  previousMillis = currentMillis;
               }        
               if (Brigthness <=0) newMove = OFF;
               Serial.println("Move : LUMI");
               Serial.println(Brigthness);
               break;     
    case OFF: clear();newMove=IDLE;               Serial.println("Move : OFF");break;
    case IDLE:Serial.println(restart);Serial.println("Move : IDLE");break;
  }
  Move = newMove;
  delay(200);
}
