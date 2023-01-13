// OLED control library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
//EEPROM library
#include <EEPROM.h>

// OLED-screen setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Button flags
bool pressedup,pressedmid,presseddown;
double presstime;

// EEPROM stored data
int storedWeight;
int pressure;
int preinfusion;

//DualCore
TaskHandle_t Core0;
TaskHandle_t Core1;

void setup() {
  Serial.begin(115200);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);

  oled_initialize();

  // This is seeing if this is the first time you are writing to the eeprom and if it is just put a 0 for now
  if (EEPROM.read(256) != 123) {
    EEPROM.write(256, 123);
    storedWeight = 0;
    Serial.println("EEPROM init");
  }else { // This code runs every time the arduino powers back on... it sets the target weight to the last weight used so that it "remembers"
    EEPROM.get(0, storedWeight);
    Serial.println("Data get");
  }

  presstime = micros();
  attachInterrupt(digitalPinToInterrupt(6), pressup, FALLING);
  attachInterrupt(digitalPinToInterrupt(7), pressmid, FALLING);
  attachInterrupt(digitalPinToInterrupt(0), pressdown, FALLING);

  xTaskCreatePinnedToCore(Core0code,"Core0",10000,NULL,1,&Core0,0);                   
  delay(500);  
  xTaskCreatePinnedToCore(Core1code,"Core1",10000,NULL,1,&Core1,1);
  delay(500);
}

void Core0code( void * pvParameters ){

  for(;;){
    delay(500);
  } 
}

void Core1code( void * pvParameters ){

  for(;;){
    delay(500); 
  }
}

void loop() {
  // put nothing
}

void pressup(){
  if(micros()-presstime>10000){
    pressedup = true;
    presstime = micros();
  }
}

void pressmid(){
  if(micros()-presstime>10000){
    pressedmid = true;
    presstime = micros();
  }
}

void pressdown(){
  if(micros()-presstime>10000){
    presseddown = true;
    presstime = micros();
  }
}

//---------------------------------------------------------------------------------------------//

void oled_initialize(){
  if (!display.begin(0x3c, true)) { // Address 0x3C for 128x32
  Serial.println(F("SSD1306 allocation failed"));
  for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(2.5);
  display.setTextColor(SH110X_WHITE); // Draw white text
  display.setCursor(18, 10);
  display.print("Silvia32");
  display.setTextSize(1);
  display.setCursor(54, 50);
  display.print("Jeremy Chang");
  display.display();
}
