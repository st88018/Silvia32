// OLED control library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
//EEPROM library
#include <EEPROM.h>

// OLED-screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Button flags
bool pressedup,pressedmid,presseddown;
double presstime;

// EEPROM stored data
int weightAddr = 0;
int tempAddr = 10;
int pressureAddr = 20;
int preinfusionAddr = 30;

int targetWeight = 0; //in gram
int targetTemp = 93; //in celsius degree
int pressure = 90; // in bar*10
int preinfusion = 0; // in seconds

//Sensors
float currentTemp;

// Mode management
int mode = 1; // Mode 0: Heating, 1: Brew, 2: Setting;
int cursurPos = 0;

//DualCore
TaskHandle_t Core0;
TaskHandle_t Core1;

void setup() {
  Serial.begin(115200);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);

  oled_initialize();

  //collect eeprom data
  EEPROM.begin(128);
  targetTemp = EEPROM.read(tempAddr);
  targetWeight = EEPROM.read(weightAddr);
  pressure = EEPROM.read(pressure);
  preinfusion = EEPROM.read(preinfusion);
  Serial.print("EEPROM updated");
  
  //Initialize manual input
  presstime = millis();
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
  float UItimer = millis();
  float OledBlinktimer = millis();
  bool Blinkstate, Blinking; 
  for(;;){
    if (millis()-UItimer > 100){
      UItimer = millis();
      oled_display();
      userinterface();
      serial_debug();
    }
  }
}

void loop() {
  // put nothing
}

void pressup(){
  if(millis()-presstime>250){
    pressedup = true;
    presstime = millis();
  }
}

void pressmid(){
  if(millis()-presstime>250){
    pressedmid = true;
    presstime = millis();
  }
}

void pressdown(){
  if(millis()-presstime>250){
    presseddown = true;
    presstime = millis();
  }
}

//---------------------------------------------------------------------------------------------//
static const unsigned char PROGMEM logo_bmp[] = {/* 0X00,0X01,0X32,0X00,0X3D,0X00, */
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X03,0X80,0X00,0X01,0XF0,0X00,0X00,0X07,0XC0,0X00,0X0F,
0XFE,0X00,0X00,0X0F,0XE0,0X00,0X1F,0XFF,0X80,0X00,0X0F,0XF0,0X00,0X7F,0XFF,0XC0,
0X00,0X0F,0XF8,0X00,0XFF,0XFF,0XE0,0X00,0X0F,0XFC,0X00,0XFF,0XFF,0XF0,0X00,0X07,
0XFE,0X01,0XFF,0XFF,0XF0,0X00,0X03,0XFF,0X03,0XFF,0XFF,0XF8,0X00,0X01,0XFF,0X83,
0XFE,0X0F,0XF8,0X00,0X00,0XFF,0XC3,0XFC,0X07,0XFC,0X00,0X00,0X7F,0XE7,0XF8,0X03,
0XFC,0X00,0X00,0X3F,0XF7,0XF8,0X01,0XFC,0X00,0X00,0X1F,0XFF,0XF8,0X01,0XFC,0X00,
0X00,0X0F,0XFF,0XF8,0X01,0XFC,0X00,0X00,0X07,0XFF,0XF8,0X03,0XFC,0X00,0X00,0X0F,
0XFF,0XF8,0X03,0XFC,0X00,0X00,0X1F,0XF7,0XF8,0X07,0XFC,0X00,0X00,0X3F,0XF7,0XF8,
0X0F,0XF8,0X00,0X00,0X3F,0XE7,0XF8,0X1F,0XF8,0X00,0X00,0X7F,0XC7,0XF8,0X1F,0XF0,
0X00,0X00,0XFF,0XC7,0XF8,0X3F,0XE0,0X00,0X01,0XFF,0X87,0XF8,0X7F,0XE0,0X00,0X03,
0XFF,0X07,0XF8,0XFF,0XC0,0X00,0X03,0XFE,0X07,0XF9,0XFF,0X80,0X00,0X07,0XFC,0X07,
0XF9,0XFF,0X00,0X00,0X07,0XFC,0X07,0XFB,0XFE,0X00,0X00,0X0F,0XF8,0X07,0XFF,0XFE,
0X00,0X00,0X0F,0XF0,0X07,0XFF,0XFC,0X00,0X00,0X0F,0XF0,0X07,0XFF,0XFC,0X00,0X00,
0X0F,0XF0,0X07,0XFF,0XFE,0X00,0X00,0X0F,0XF0,0X07,0XFF,0XFF,0X00,0X00,0X0F,0XF0,
0X07,0XFB,0XFF,0X80,0X00,0X0F,0XF0,0X07,0XF9,0XFF,0XC0,0X00,0X0F,0XF8,0X0F,0XF0,
0XFF,0XE0,0X00,0X07,0XFE,0X3F,0XF0,0X7F,0XF0,0X00,0X07,0XFF,0XFF,0XF0,0X3F,0XF8,
0X00,0X03,0XFF,0XFF,0XE0,0X1F,0XF8,0X00,0X03,0XFF,0XFF,0XC0,0X0F,0XFC,0X00,0X01,
0XFF,0XFF,0X80,0X07,0XFC,0X00,0X00,0XFF,0XFF,0X00,0X03,0XFC,0X00,0X00,0X3F,0XFE,
0X00,0X01,0XFC,0X00,0X00,0X1F,0XF8,0X00,0X00,0XF8,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0XF3,0XB2,0X7F,0X27,0X00,0X00,0X00,0XDB,0XBB,0XFF,0X3F,0X80,0X00,0X00,0XFE,
0XBF,0X8F,0X38,0XC0,0X00,0X00,0XF7,0XFF,0X8F,0X38,0XC0,0X00,0X00,0XFF,0XF7,0XFF,
0XFF,0X80,0X00,0X00,0XDC,0X72,0X7F,0XE7,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X60,0XE1,0XE3,0X43,0X80,0X00,0X00,0XD0,0XE1,0X63,0X42,0X80,0X00,0X00,
0X70,0XA0,0XC1,0XC2,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,};

void oled_initialize(){
  if (!display.begin(0x3c, true)) { // Address 0x3C for 128x32
  Serial.println(F("SSD1306 allocation failed"));
  for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(2.75);
  display.setTextColor(SH110X_WHITE); // Draw white text
  display.setCursor(15, 10);
  display.print("Silvia32");
  display.setTextSize(1);
  display.setCursor(54, 50);
  display.print("Jeremy Chang");
  display.display();
  delay(2000);
  display.clearDisplay();
  display.drawBitmap(40,2,logo_bmp, 50, 61, 1);
  display.display();
}

void oled_display(){
  if(mode == 0 && millis() > 5000){
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE); // Draw white text
    display.setCursor(25, 10);
    display.print("Heating");
    display.setTextSize(1);
    display.setCursor(5, 40);
    display.print("Target:");
    display.setCursor(15, 50);
    display.print(targetTemp);
    display.setCursor(80, 40);
    display.print("Temp(C):");
    display.setCursor(90, 50);
    display.print(currentTemp);
    display.display();
    if(currentTemp>80) mode = 1;
  }
  if(mode == 1){
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(5, 5);
    
  }
}

void userinterface(){
  if(mode == 0){
    if(pressedup){
      pressedup = false;
      targetTemp++;
    }
    if(presseddown){
      presseddown = false;
      targetTemp--;
    }
    if(pressedmid){
      pressedmid = false;
      EEPROM.write(tempAddr, targetTemp);
      EEPROM.commit();
      Serial.println("TempSaved");
    }
  }
}

void serial_debug(){
  Serial.print("Mode:");Serial.print(mode);
  Serial.print(" Target Temp:");Serial.print(targetTemp);Serial.print(" Current Temp:");Serial.println(currentTemp);
  // Serial.println(UItimer-millis());
}