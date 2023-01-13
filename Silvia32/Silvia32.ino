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

void setup() {
  Serial.begin(115200);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);

  oled_initialize();

  // attachInterrupt(7, pressedup, FALLING);
  // attachInterrupt(0, pressedmid, FALLING);
  // attachInterrupt(6, presseddown, FALLING);

  // xTaskCreatePinnedToCore(Task1code,"Task1",10000,NULL,1,&Task1,0);                   
  // delay(500);  
  // xTaskCreatePinnedToCore(Task2code,"Task2",10000,NULL,1,&Task2,1);
  // delay(500);
}

void Task1code( void * pvParameters ){

  for(;;){

  } 
}

void Task2code( void * pvParameters ){

  for(;;){

  }
}

void loop() {
  // put nothing

}

//---------------------------------------------------------------------------------------------//

void oled_initialize(){
  if (!display.begin(0x3c, true)) { // Address 0x3C for 128x32
  Serial.println(F("SSD1306 allocation failed"));
  for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE); // Draw white text
  display.setCursor(0, 0);
  display.print("Silvia32");
  display.display();
}
