// OLED control library
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
//EEPROM library
#include <EEPROM.h>
//Encoder library
#include "AiEsp32RotaryEncoder.h"


// OLED-screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Encoder
#define ROTARY_ENCODER_BUTTON_PIN 13
#define ROTARY_ENCODER_A_PIN 14
#define ROTARY_ENCODER_B_PIN 21
bool pressedmid1, pressedmid, longpressmid, presseddown, pressedup;
double presstime, pressmidtime;
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, 4);
void IRAM_ATTR readEncoderISR(){
	rotaryEncoder.readEncoder_ISR();
}

// EEPROM stored data
int weightAddr = 0;
int tempAddr = 10;
int pressureAddr = 20;
int preinfusionAddr = 30;
int Kp_tempAddr = 40;
int Ki_tempAddr = 50;
int Kd_tempAddr = 60;
int Kp_pressureAddr = 70;
int Ki_pressureAddr = 80;
int Kd_pressureAddr = 90;

int targetWeight = 32;  //in gram
int targetTemp = 93;   //in celsius degree
int targetpressure = 90;     // in bar*10
int targetpreinfusion = 10;   // in seconds

//Sensors
float currentTemp;
float currentWeight;
float currentPressure;

// Mode management
int mode = 4;  // Mode 0: Heating, 1: Brew, 2: Brewing, 3: Clean, 4: Setting;
int cursurPos = 0;
bool cursurPosSelected = false;

bool brewstarted = 0;
double BrewstartedTime; // in millis
float brewpercent = 0; //something from 0~1
int cleancount = 0;
//Controllers
double Temp_Integral,Temp_last_error,Pressure_Integral,Pressure_last_error;

float Kp_temp = 10;
float Ki_temp = 0;
float Kd_temp = 0;
float Kp_pressure = 10;
float Ki_pressure = 0;
float Kd_pressure = 0;

//Blink
int blinkcounter = 0;
bool blinkstate = 0;

//DualCore
TaskHandle_t Core0;
TaskHandle_t Core1;
float code0timer,code1timer;

void setup() {
  Serial.begin(115200);
  pinMode(1, OUTPUT);
  digitalWrite(1, HIGH);

  oled_initialize();

  /*collect eeprom data*/
  EEPROM.begin(128);
  saveParameters();
  targetTemp = EEPROM.read(tempAddr);
  targetWeight = EEPROM.read(weightAddr);
  targetpressure = EEPROM.read(pressureAddr);
  targetpreinfusion = EEPROM.read(preinfusionAddr);
  Kp_temp = EEPROM.read(Kp_tempAddr);
  Ki_temp = EEPROM.read(Ki_tempAddr);
  Kd_temp = EEPROM.read(Kd_tempAddr);
  Kp_pressure = EEPROM.read(Kp_pressureAddr);
  Ki_pressure = EEPROM.read(Ki_pressureAddr);
  Kd_pressure = EEPROM.read(Kd_pressureAddr);
  Serial.print("EEPROM updated");

  /*Initialize manual input*/
  attachInterrupt(digitalPinToInterrupt(ROTARY_ENCODER_BUTTON_PIN), pressmid, FALLING);
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setBoundaries(-1000, 1000, false);
  rotaryEncoder.disableAcceleration();
  
  
  /*Initialize dual core*/
  xTaskCreatePinnedToCore(Core0code, "Core0", 10000, NULL, 1, &Core0, 0);
  delay(500);
  xTaskCreatePinnedToCore(Core1code, "Core1", 10000, NULL, 1, &Core1, 1);
  delay(500);
}
void Core0code(void* pvParameters) {
  for (;;) {
    // serial_debug();
    delay(500);
  }
}
void Core1code(void* pvParameters) {
  for (;;) {
    longpresschecker();
    if (millis() - code1timer > 50) {
      code1timer = millis();
      rotary_loop();
      oled_display();
      userinterface();
      blinkcontroller();
    }
  }
}
void loop() {
  // put nothing
}
//---------------------------------------------------------------------------------------------//
void pressmid() {
  if (millis() - presstime > 350) {
    pressedmid1 = true;
    pressmidtime = millis();
    serial_debug();
  }
}
void longpresschecker(){
  if(pressedmid1 && digitalRead(ROTARY_ENCODER_BUTTON_PIN)){
    pressedmid = true;
    pressedmid1 = false;
    presstime = millis();
  }
  if(pressedmid1 && millis()-pressmidtime>1000){
    longpressmid = true;
    pressedmid1 = false;
    presstime = millis(); 
  }
}
void rotary_loop(){
	//dont print anything unless value changed
	if (rotaryEncoder.encoderChanged())
	{
		Serial.print("Value: ");
		Serial.println(rotaryEncoder.readEncoder());
	}
}
//---------------------------------------------------------------------------------------------//

static const unsigned char PROGMEM logo_bmp[] = {
  /* 0X00,0X01,0X32,0X00,0X3D,0X00, */
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X03,
  0X80,
  0X00,
  0X01,
  0XF0,
  0X00,
  0X00,
  0X07,
  0XC0,
  0X00,
  0X0F,
  0XFE,
  0X00,
  0X00,
  0X0F,
  0XE0,
  0X00,
  0X1F,
  0XFF,
  0X80,
  0X00,
  0X0F,
  0XF0,
  0X00,
  0X7F,
  0XFF,
  0XC0,
  0X00,
  0X0F,
  0XF8,
  0X00,
  0XFF,
  0XFF,
  0XE0,
  0X00,
  0X0F,
  0XFC,
  0X00,
  0XFF,
  0XFF,
  0XF0,
  0X00,
  0X07,
  0XFE,
  0X01,
  0XFF,
  0XFF,
  0XF0,
  0X00,
  0X03,
  0XFF,
  0X03,
  0XFF,
  0XFF,
  0XF8,
  0X00,
  0X01,
  0XFF,
  0X83,
  0XFE,
  0X0F,
  0XF8,
  0X00,
  0X00,
  0XFF,
  0XC3,
  0XFC,
  0X07,
  0XFC,
  0X00,
  0X00,
  0X7F,
  0XE7,
  0XF8,
  0X03,
  0XFC,
  0X00,
  0X00,
  0X3F,
  0XF7,
  0XF8,
  0X01,
  0XFC,
  0X00,
  0X00,
  0X1F,
  0XFF,
  0XF8,
  0X01,
  0XFC,
  0X00,
  0X00,
  0X0F,
  0XFF,
  0XF8,
  0X01,
  0XFC,
  0X00,
  0X00,
  0X07,
  0XFF,
  0XF8,
  0X03,
  0XFC,
  0X00,
  0X00,
  0X0F,
  0XFF,
  0XF8,
  0X03,
  0XFC,
  0X00,
  0X00,
  0X1F,
  0XF7,
  0XF8,
  0X07,
  0XFC,
  0X00,
  0X00,
  0X3F,
  0XF7,
  0XF8,
  0X0F,
  0XF8,
  0X00,
  0X00,
  0X3F,
  0XE7,
  0XF8,
  0X1F,
  0XF8,
  0X00,
  0X00,
  0X7F,
  0XC7,
  0XF8,
  0X1F,
  0XF0,
  0X00,
  0X00,
  0XFF,
  0XC7,
  0XF8,
  0X3F,
  0XE0,
  0X00,
  0X01,
  0XFF,
  0X87,
  0XF8,
  0X7F,
  0XE0,
  0X00,
  0X03,
  0XFF,
  0X07,
  0XF8,
  0XFF,
  0XC0,
  0X00,
  0X03,
  0XFE,
  0X07,
  0XF9,
  0XFF,
  0X80,
  0X00,
  0X07,
  0XFC,
  0X07,
  0XF9,
  0XFF,
  0X00,
  0X00,
  0X07,
  0XFC,
  0X07,
  0XFB,
  0XFE,
  0X00,
  0X00,
  0X0F,
  0XF8,
  0X07,
  0XFF,
  0XFE,
  0X00,
  0X00,
  0X0F,
  0XF0,
  0X07,
  0XFF,
  0XFC,
  0X00,
  0X00,
  0X0F,
  0XF0,
  0X07,
  0XFF,
  0XFC,
  0X00,
  0X00,
  0X0F,
  0XF0,
  0X07,
  0XFF,
  0XFE,
  0X00,
  0X00,
  0X0F,
  0XF0,
  0X07,
  0XFF,
  0XFF,
  0X00,
  0X00,
  0X0F,
  0XF0,
  0X07,
  0XFB,
  0XFF,
  0X80,
  0X00,
  0X0F,
  0XF0,
  0X07,
  0XF9,
  0XFF,
  0XC0,
  0X00,
  0X0F,
  0XF8,
  0X0F,
  0XF0,
  0XFF,
  0XE0,
  0X00,
  0X07,
  0XFE,
  0X3F,
  0XF0,
  0X7F,
  0XF0,
  0X00,
  0X07,
  0XFF,
  0XFF,
  0XF0,
  0X3F,
  0XF8,
  0X00,
  0X03,
  0XFF,
  0XFF,
  0XE0,
  0X1F,
  0XF8,
  0X00,
  0X03,
  0XFF,
  0XFF,
  0XC0,
  0X0F,
  0XFC,
  0X00,
  0X01,
  0XFF,
  0XFF,
  0X80,
  0X07,
  0XFC,
  0X00,
  0X00,
  0XFF,
  0XFF,
  0X00,
  0X03,
  0XFC,
  0X00,
  0X00,
  0X3F,
  0XFE,
  0X00,
  0X01,
  0XFC,
  0X00,
  0X00,
  0X1F,
  0XF8,
  0X00,
  0X00,
  0XF8,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0XF3,
  0XB2,
  0X7F,
  0X27,
  0X00,
  0X00,
  0X00,
  0XDB,
  0XBB,
  0XFF,
  0X3F,
  0X80,
  0X00,
  0X00,
  0XFE,
  0XBF,
  0X8F,
  0X38,
  0XC0,
  0X00,
  0X00,
  0XF7,
  0XFF,
  0X8F,
  0X38,
  0XC0,
  0X00,
  0X00,
  0XFF,
  0XF7,
  0XFF,
  0XFF,
  0X80,
  0X00,
  0X00,
  0XDC,
  0X72,
  0X7F,
  0XE7,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X60,
  0XE1,
  0XE3,
  0X43,
  0X80,
  0X00,
  0X00,
  0XD0,
  0XE1,
  0X63,
  0X42,
  0X80,
  0X00,
  0X00,
  0X70,
  0XA0,
  0XC1,
  0XC2,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
  0X00,
};
void oled_initialize() {
  if (!display.begin(0x3c, true)) {  // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    // for (;;);  // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextSize(2.75);
  display.setTextColor(SH110X_WHITE);  // Draw white text
  display.setCursor(15, 10);
  display.print("Silvia32");
  display.setTextSize(1);
  display.setCursor(54, 50);
  display.print("Jeremy Chang");
  display.display();
  delay(2700);
  display.clearDisplay();
  display.drawBitmap(40, 2, logo_bmp, 50, 61, 1);
  display.display();
}
void oled_display() {
  display.clearDisplay();
  if (mode == 0 && millis() > 4000) {
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
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
    if (currentTemp > 80) mode = 1;
  }
  if (mode == 1) {
    displayTemp();
    displayPress();
    displayWeight();
    displayPreinfusion();
  }
  if(mode == 2){
    displayTemp();
    displayPress();
    displayWeight();
    displayPreinfusion();
  }if(mode ==3){
    display.drawRect(0, 0, 128, 56, SH110X_WHITE);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(8, 15);
    display.print("Press Brew to start");
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(80, 32);
    display.print(cleancount);
  }if(mode ==4){
    displaySettings();
  }
  displayModeColumn();
  display.display();
}
void userinterface() {
  if (mode == 0) {
    if (pressedup) {
      pressedup = false;
      targetTemp++;
    }
    if (presseddown) {
      presseddown = false;
      targetTemp--;
    }
    if (pressedmid) {
      pressedmid = false;
      EEPROM.write(tempAddr, targetTemp);
      EEPROM.commit();
      Serial.println("TempSaved");
    }
  }
  if (mode == 1){
    if(longpressmid){
      mode = 3;
      cursurPos = 0;
      longpressmid = false;
      pressedup = false;
      pressedmid = false;
      presseddown = false;
      cursurPos = 0;
      saveParameters();
    }
    if(pressedmid){
      pressedmid = false;
      if(cursurPos != 0) cursurPosSelected = !cursurPosSelected;
    }
    if( pressedup && cursurPosSelected){
      if(cursurPos == 1) targetTemp++;
      if(cursurPos == 2) targetWeight++;
      if(cursurPos == 3) targetpreinfusion++;
      pressedup = false;
    }
    if(pressedup){
      if(cursurPos == 0){
        cursurPos = 3;
      }else{
        cursurPos--;
      }
      if(cursurPos == 0) cursurPos = 3;
      pressedup = false;      
    }
    if (millis() - presstime > 10000){ //stop selecting while not using
      cursurPos = 0;
      saveParameters();
    }
    if( presseddown && cursurPosSelected){
      if(cursurPos == 1) targetTemp--;
      if(cursurPos == 2) targetWeight--;
      if(cursurPos == 3) targetpreinfusion--;
      presseddown = false;
    }
    if(presseddown){
      if(cursurPos == 0){
        cursurPos = 1;
      }else{
        cursurPos++;
      }
      if(cursurPos == 4) cursurPos = 1;
      presseddown = false;      
    }
    if (millis() - presstime > 10000){ //stop selecting while not using
      cursurPos = 0;
      saveParameters();
    }
  }
  if (mode == 3){
    if(longpressmid){
      mode = 4;
      cursurPos = 0;
      longpressmid = false;
      pressedup = false;
      pressedmid = false;
      presseddown = false;
    }
  }
  if (mode == 4){
    if(longpressmid){
      mode = 1;
      cursurPos = 0;
      longpressmid = false;
      pressedup = false;
      pressedmid = false;
      presseddown = false;
    }
    if(pressedmid && cursurPos < 4){
      pressedmid = false;
      if(cursurPos == 0){ cursurPos = 4; cursurPosSelected = false;} // Temp P,I,D
      if(cursurPos == 1){ cursurPos = 8; cursurPosSelected = false;} // Pressure P,I,D
      if(cursurPos == 2){ cursurPos = 10; cursurPosSelected = false;} // Preinfusion pressure
      if(cursurPos == 3){ cursurPos = 11; cursurPosSelected = false;} // HX711 scale
      cursurPosSelected = false;
    }
    if(pressedmid && cursurPos > 3){
      pressedmid = false;
      if(cursurPos > 3 && cursurPos < 7) cursurPosSelected = !cursurPosSelected;
      if(cursurPos == 7){ cursurPos = 0; cursurPosSelected = false;}
    }
    if(pressedup){
      if(cursurPos < 4){
        cursurPos--;
        constrain(cursurPos,0,3);
        }
      if(cursurPos > 3 && cursurPos < 7 && !cursurPosSelected){
        cursurPos--;
        constrain(cursurPos,4,6);
      } 
      if(cursurPos == 4 && cursurPosSelected){Kp_temp = Kp_temp+0.1; constrain(Kp_temp, 0, 100);}
      if(cursurPos == 5 && cursurPosSelected){Ki_temp = Ki_temp+0.1; constrain(Kp_temp, 0, 100);}
      if(cursurPos == 6 && cursurPosSelected){Kd_temp = Kd_temp+0.1; constrain(Kp_temp, 0, 100);}
      pressedup = false;
    }
    if(presseddown){
      if(cursurPos < 4){
        cursurPos++;
        constrain(cursurPos,0,3);
        }
      if(cursurPos > 3 && cursurPos < 7 && !cursurPosSelected){
        cursurPos++;
        constrain(cursurPos,4,6);
      }
      if(cursurPos == 4 && cursurPosSelected){Kp_temp = Kp_temp-0.1; constrain(Kp_temp, 0, 100);}
      if(cursurPos == 5 && cursurPosSelected){Ki_temp = Ki_temp-0.1; constrain(Kp_temp, 0, 100);}
      if(cursurPos == 6 && cursurPosSelected){Kd_temp = Kd_temp-0.1; constrain(Kp_temp, 0, 100);}
      presseddown = false;
    }
  }
}
void serial_debug() {
  Serial.print("Mode:");
  Serial.print(mode);
  Serial.print(" cursurPos:");
  Serial.print(cursurPos);
  Serial.print(" TargetTemp:");
  Serial.print(targetTemp);
  Serial.print(" CurrentTemp:");
  Serial.println(currentTemp);
  Serial.print(presseddown);
  Serial.print(" cursurPosSelected:");
  Serial.println(cursurPosSelected);
}
void displayModeColumn(){
  int BrewPos[2] = {8,56};
  int cleanPos[2] = {41,56};
  int settingPos[2] = {80,56};
  display.setTextSize(1);
  if(mode == 1){ // Wait for Brew
    display.fillTriangle(0,55, 4,55, 4,64, SH110X_WHITE);
    display.fillRect(5,55,3,9, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(BrewPos[0],BrewPos[1]);
    display.print("Brew");
    display.fillRect(32,55,2,9, SH110X_WHITE);
    display.fillTriangle(34,64, 34,55, 38,55, SH110X_WHITE);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(cleanPos[0], cleanPos[1]);
    display.print("clean");
    display.drawLine(settingPos[0]-8,64, settingPos[0]-3,55, SH110X_WHITE);
    display.setCursor(settingPos[0], settingPos[1]);
    display.print("setting");
    display.drawLine(123,64, 128,55, SH110X_WHITE);
  }
  if(mode == 2){ //Brewing
    display.fillTriangle(0,55, 4,55, 4,64, SH110X_WHITE);
    display.fillRect(5,55,3,9, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(BrewPos[0],BrewPos[1]);
    display.print("Brew");
    display.fillRect(32,55,2,9, SH110X_WHITE);
    display.fillTriangle(34,64, 34,55, 38,55, SH110X_WHITE);
    displayBrewState();
  }
  if(mode == 3){ //Clean
    display.setTextColor(SH110X_WHITE);
    display.drawLine(0,55, 5,64,SH110X_WHITE);
    display.setCursor(BrewPos[0],BrewPos[1]);
    display.print("Brew");
    display.fillTriangle(34,55, 38,55, 38,64, SH110X_WHITE);
    display.fillRect(38,55,3,9, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(cleanPos[0], cleanPos[1]);
    display.print("clean");
    display.fillRect(70,55,2,9, SH110X_WHITE);
    display.fillTriangle(72,64, 72,55, 76,55, SH110X_WHITE);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(settingPos[0], settingPos[1]);
    display.print("setting");
    display.drawLine(123,64, 128,55, SH110X_WHITE);
  }
  if(mode == 4){ //Setting
    display.setTextColor(SH110X_WHITE);
    display.drawLine(0,55, 5,64,SH110X_WHITE);
    display.setCursor(BrewPos[0],BrewPos[1]);
    display.print("Brew");
    display.drawLine(cleanPos[0]-8,55, cleanPos[0]-3,64, SH110X_WHITE);
    display.setCursor(cleanPos[0], cleanPos[1]);
    display.print("clean");
    display.fillTriangle(73,55, 77,55, 77,64, SH110X_WHITE);
    display.fillRect(77,55,3,9, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.setCursor(settingPos[0], settingPos[1]);
    display.print("setting");
    display.fillRect(122,55,1,9, SH110X_WHITE);
    display.fillTriangle(123,64, 123,55, 127,55, SH110X_WHITE);
  }
}
void displayBrewState() {  //0 to 1
  display.drawLine(34,63, 123,63, SH110X_WHITE);
  display.drawLine(123,64, 128,55, SH110X_WHITE);
  int i;
  if(brewpercent == 0){
    if(blinkstate){
      i = 0.05*86;
      display.fillRect(35,55,i,9, SH110X_WHITE); //35~121
      display.fillTriangle(i+35,64, i+35,55, i+39,55, SH110X_WHITE);
    }else{
      i = 0;
      display.fillRect(35,55,i,9, SH110X_WHITE); //35~121
      display.fillTriangle(i+35,64, i+35,55, i+39,55, SH110X_WHITE);
    }
  }else{
    i = brewpercent*86;
    display.fillRect(35,55,i,9, SH110X_WHITE); //35~121
    display.fillTriangle(i+35,64, i+35,55, i+39,55, SH110X_WHITE);
  }
}
void displayTemp() {
  display.drawRect(0, 0, 64, 28, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);  // Draw white text
  display.setCursor(2, 2);
  display.print("Temp");
  display.print(" (");
  if(cursurPos == 1){
    if(cursurPosSelected && !blinkstate){
    
    }else{
      display.drawLine(37,2,37,9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    }
  }
  display.print(targetTemp);
  display.setTextColor(SH110X_WHITE);
  display.print(")");
  display.setCursor(15, 12);
  display.setTextSize(2);
  display.print(currentTemp,1);
}
void displayPress() {
  display.drawRect(0, 28, 64, 28, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);  // Draw white text
  display.setCursor(2, 30);
  display.print("Pressure");
  display.setTextSize(2);
  display.setCursor(15, 40);
  display.print(currentPressure,1);
}
void displayWeight() {
  display.drawRect(64, 0, 64, 28, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);  // Draw white text
  display.setCursor(66, 2);
  display.print("Weight");
  display.print("(");
  if(cursurPos == 2){
    if(cursurPosSelected && !blinkstate){
    
    }else{
      display.drawLine(107,2,107,9, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    }
  }
  display.print(targetWeight);
  display.setTextColor(SH110X_WHITE);
  display.print(")");
  display.setTextSize(2);
  display.setCursor(79, 12);
  display.print(currentWeight,1);
}
void displayPreinfusion() {
  display.drawRect(64, 28, 64, 28, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);  // Draw white text
  display.setCursor(66, 30);
  if(mode == 1){
    display.print("Preinfuse");
  }else{
    display.print("Time");
  }
  display.setTextSize(2);
  display.setCursor(79, 40);
  if(cursurPos == 3){
    if(cursurPosSelected && !blinkstate){
    
    }else{
      display.drawLine(79,40,79,54, SH110X_WHITE);
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    }
  }
  if(mode == 1){
    display.print(targetpreinfusion);
  }else{ //while brewing
    double currentTime = -targetpreinfusion+((millis()-BrewstartedTime)*0.001);
    if (currentTime > 99.9) currentTime = 99.9;
    display.print(abs(currentTime),1);
  }
}
void displaySettings(){
  display.drawRect(0, 0, 128, 56, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  if (cursurPos == 0) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(4, 4);
  if (cursurPos < 4) display.print("Temperature PID");
  display.setTextColor(SH110X_WHITE);
  if (cursurPos == 1) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(4, 15);
  if (cursurPos < 4) display.print("Pressure PID");
  display.setTextColor(SH110X_WHITE);
  if (cursurPos == 2) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(4, 26);
  if (cursurPos < 4) display.print("Preinfuse pressure");
  display.setTextColor(SH110X_WHITE);
  if (cursurPos == 3) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
  display.setCursor(4, 37);
  if (cursurPos < 4) display.print("HX711 scale");
  if (cursurPos > 3 && cursurPos < 8){ //Tunung Temp PID
    display.setTextSize(1);
    display.setCursor(4, 4);
    display.setTextColor(SH110X_WHITE);
    display.print("Temperature PID");
    display.setTextSize(2);
    display.setCursor(5, 25);
    display.print("P");
    display.setCursor(45, 25);
    display.print("I");
    display.setCursor(85, 25);
    display.print("D");
    display.setTextSize(1);
    display.setCursor(20, 30);
    display.setTextColor(SH110X_WHITE);
    if (cursurPos == 4) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(Kp_temp,1);
    display.setCursor(60, 30);
    display.setTextColor(SH110X_WHITE);
    if (cursurPos == 5) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(Ki_temp,1);
    display.setCursor(100, 30);
    display.setTextColor(SH110X_WHITE);
    if (cursurPos == 6) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(Kd_temp,1);
    display.setCursor(102, 45);
    display.setTextColor(SH110X_WHITE);
    if (cursurPos == 7) display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print("back");
  }
  if (cursurPos == 8){ //Tunung Pressure PID
    
  }
  if (cursurPos == 10){ //Tunung Preinfusion pressure
    
  }
  if (cursurPos == 11){ //Tunung HX711
    
  }
}
void blinkcontroller(){
  if (blinkcounter<5) {
    blinkcounter++;
  }else{
    blinkcounter = 0;
    blinkstate = !blinkstate;
  }
}
void saveParameters(){
  EEPROM.write(pressureAddr, targetpressure);
  EEPROM.write(weightAddr, targetWeight);
  EEPROM.write(preinfusionAddr, targetpreinfusion);
  EEPROM.write(tempAddr, targetTemp);
  EEPROM.write(Kp_tempAddr, Kp_temp);
  EEPROM.write(Ki_tempAddr, Ki_temp);
  EEPROM.write(Kd_tempAddr, Kd_temp);
  EEPROM.write(Kp_pressureAddr, Kp_pressure);
  EEPROM.write(Ki_pressureAddr, Ki_pressure);
  EEPROM.write(Kd_pressureAddr, Kd_pressure);
  EEPROM.commit();
}
double Temp_controller(double targetTemp, double currentTemp,double dT){
  double error = targetTemp-currentTemp;
  Temp_Integral += error*dT;
  Temp_Integral = constrain(Temp_Integral, -1,1);
  double Tempderivative = (error-Temp_last_error)/(dT+1e-10);
  Temp_last_error = error;
  double output = error*Kp_temp+Temp_Integral*Ki_temp+Tempderivative*Kd_temp;
  return(output);
}
double Pressure_controller(double targetPressure, double currentPressure,double dT){
  double error = targetPressure-currentPressure;
  Pressure_Integral += error*dT;
  Pressure_Integral = constrain(Pressure_Integral, -1,1);
  double Pressurederivative = (error-Pressure_last_error)/(dT+1e-10);
  Pressure_last_error = error;
  double output = error*Kp_pressure+Pressure_Integral*Ki_pressure+Pressurederivative*Kd_pressure;
  return(output);
}
void PCA9685_output(){
  //Temperature SSR

  //Solenoid SSR

  //Pressure dimmer
  
}
void ADS1115_input(){

}
