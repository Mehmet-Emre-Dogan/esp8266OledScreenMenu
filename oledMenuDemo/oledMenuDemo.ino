#define DEBUG_ENABLED true
/* OLED Display */
// See the link below for extra info
// https://www.instructables.com/OLED-I2C-Display-ArduinoNodeMCU-Tutorial/
// https://www.circuitgeeks.com/oled-display-arduino/
// https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
// https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives

// Font generator
// https://oleddisplay.squix.ch/#/home

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts\FreeSans18pt7b.h>
#include <Fonts\FreeSans12pt7b.h>
#include "dialog13.h"
#include "dialog11.h"
#include "dialog9.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

/* Rotary Encoder */
// See the link below for extra info
// https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
// https://dronebotworkshop.com/rotary-encoders-arduino/
#define ROTARY_CLK D6
#define ROTARY_DT D7
#define ROTARY_BTN D3

/* External i2c EEPROM */
// External EEPROM reading-writing functions are adopted from the link below:
// https://dronebotworkshop.com/eeprom-arduino/
// https://arduino-projects4u.com/arduino-24c16/

#include <SoftwareI2C.h>
#define EEPROM_ADDR 0x50
#define EEPROM_SCL D4
#define EEPROM_SDA D5
#define EEPROM_TYPE "24C08" // The other model I have tested is "24C32"
SoftwareI2C softwarei2c;
bool configWillBeSaved = false;

void ICACHE_RAM_ATTR checkRotary();
void ICACHE_RAM_ATTR handleBtn();
void ICACHE_RAM_ATTR handleBtn_old();

/******* CONSTANTS *******/
#define MENU_COUNT 6

/******* Global Variables *******/

/* OLED Display */
String titles[MENU_COUNT] = {"Brightness", "Mode", "Sensivity", "Threshold", "Second Indicator", "Save Config."};
bool titleOrValue = 0; // 0: title selected, 1: value selected
int counter = 0; // titleIndex_x2
int titleIndex = 0;
int varArr_x2[MENU_COUNT - 1] = {511, 0, 8, 20, 2}; // int brightness_x2 = 511, vuMode_x2 = 0, sensivity_x2 = 8, threshold_x2 = 20, secsInd_x2 = 2;
uint8_t varArr[MENU_COUNT - 1] = {255, 0, 4, 10, 1}; // uint8_t brightness = 255, vuMode = 0, sensivity = 4, threshold = 10, bool secsInd = 1;
String boolArr[] = {"disabled", "enabled"};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
byte displayUpdateIterCou = 0;

/* Rotary Encoder */
int currentStateCLK;
int previousStateCLK; 

void normalizeVars(){
  for(int i = 0; i<5; i++){
    varArr[i] = (uint8_t) (varArr_x2[i]/2);
  }
    
  /*
  brightness = (uint8_t) (brightness_x2/2);
  vuMode = (uint8_t) (vuMode_x2/2);
  sensivity = (uint8_t) (sensivity_x2/2);
  threshold = (uint8_t) (threshold_x2/2);
  secsInd = (bool) (secsInd_x2/2);
  */
}

void enableInterrupts(){
  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK), checkRotary, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_DT), checkRotary, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_BTN), handleBtn, FALLING);
}

void disableInterrupts(){
  detachInterrupt(digitalPinToInterrupt(ROTARY_CLK) );
  detachInterrupt(digitalPinToInterrupt(ROTARY_DT) );
  detachInterrupt(digitalPinToInterrupt(ROTARY_BTN) );
}

void initRotary() { 
  pinMode(ROTARY_CLK,INPUT);
  pinMode(ROTARY_DT,INPUT);
  pinMode(ROTARY_BTN, INPUT_PULLUP);
  previousStateCLK = digitalRead(ROTARY_CLK);  
}

void checkRotary(){
  if (titleOrValue){
    // Read the current state of inputCLK
    currentStateCLK = digitalRead(ROTARY_CLK);
    // If the previous and the current state of the ROTARY_CLK are different then a pulse has occured
    if (currentStateCLK != previousStateCLK){  
      if (digitalRead(ROTARY_DT) != currentStateCLK){   // If the inputDT state is different than the ROTARY_CLK state then the encoder is rotating counterclockwise
        varArr_x2[titleIndex] --; 
      } 
      else{  // Encoder is rotating clockwise
        varArr_x2[titleIndex] ++; 
      }
    } 
    // Update previousStateCLK with the current state
    previousStateCLK = currentStateCLK;
  
    normalizeVars();
  }

  else{
    // Read the current state of inputCLK
    currentStateCLK = digitalRead(ROTARY_CLK);
    // If the previous and the current state of the ROTARY_CLK are different then a pulse has occured
    if (currentStateCLK != previousStateCLK){  
      if (digitalRead(ROTARY_DT) != currentStateCLK){   // If the inputDT state is different than the ROTARY_CLK state then the encoder is rotating counterclockwise
        counter --; 
      } 
      else{  // Encoder is rotating clockwise
        counter ++; 
      }
    } 
    // Update previousStateCLK with the current state
    previousStateCLK = currentStateCLK;

    titleIndex = (counter/2);
    titleIndex %= MENU_COUNT;
    if (titleIndex < 0){
      titleIndex = MENU_COUNT - 1;
      counter = (MENU_COUNT - 1)*2;
    }
     
  }

  displayUpdateIterCou = 0;
}

void handleBtn(){
  disableInterrupts();
  delay(10);
  
  while(!digitalRead(ROTARY_BTN)) // wait until the btn released
    delay(2);
  
  if(titleIndex == (MENU_COUNT - 1) ){ // If the menu on "save config"
    configWillBeSaved = true;
  }
  else{
    titleOrValue = !titleOrValue; // toggle
  }

  displayUpdateIterCou = 0; // trigger display refresh
  enableInterrupts();
}

void handleBtn_old(){
  if(titleIndex == (MENU_COUNT - 1) ){ // If the menu on "save config"
    disableInterrupts();
    saveConfig();
    enableInterrupts();
  }
  else{
    titleOrValue = !titleOrValue; // toggle
    displayUpdateIterCou = 0; // trigger display refresh
  }

}

void saveConfig(){
  if(DEBUG_ENABLED){
    Serial.println("Started to saving configuration");
  }
  
  display.clearDisplay();
  display.fillRect(0, 0, 128, 16, WHITE);
  display.setFont(&Dialog_plain_13);
  display.setCursor(1, 12);
  display.setTextColor(BLACK); // 'inverted' text
  display.println("Please wait");
  display.setCursor(1, 25);
  display.setFont(&Dialog_plain_11);
  display.setTextColor(WHITE);
  display.println("Dumping configur-ation to external\nEEPROM...");
  display.display();

  dumpEEPROM();
  delay(100);

  display.clearDisplay();
  display.fillRect(0, 0, 128, 16, WHITE);
  display.setFont(&Dialog_plain_13);
  display.setCursor(1, 12);
  display.setTextColor(BLACK); // 'inverted' text
  display.println("Success");
  display.setCursor(1, 25);
  display.setFont(&Dialog_plain_9);
  display.setTextColor(WHITE);
  display.println("EEPROM dumping pro-cess finished.\nThis message will dis-appear in 3 seconds.");
  display.display();
  delay(3000);

  configWillBeSaved = false;
  if(DEBUG_ENABLED){
    Serial.println("Configuration saved");
  }
  
}

void initDisplay(){
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setFont();
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
}

void updateDisplay(){
  display.clearDisplay();
    
  // set text
  switch(titleIndex){
    case 4:
      // 0: title selected, 1: value selected
      // BLACK: 0, WHITE: 1
      
      display.fillRect(0, 0, 128, 16, !titleOrValue);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(titleOrValue); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Enabled/Disabled");
    
      display.fillRect(0, 36, 128, 27, titleOrValue);
      display.setCursor(1, 61);
      display.setFont(&FreeSans18pt7b);
      display.setTextColor(!titleOrValue); // 'inverted' text
      display.println( boolArr[(varArr[titleIndex] % 2)]);
    break;

    case 5:  
      display.fillRect(0, 0, 128, 16, WHITE);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(BLACK); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Click to save");
    break;
    
    default:
      // 0: title selected, 1: value selected
      // BLACK: 0, WHITE: 1
      
      display.fillRect(0, 0, 128, 16, !titleOrValue);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(titleOrValue); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Range: [0-255]");
    
      display.fillRect(0, 36, 128, 27, titleOrValue);
      display.setCursor(1, 61);
      display.setFont(&FreeSans18pt7b);
      display.setTextColor(!titleOrValue); // 'inverted' text
      display.println((uint8_t) (varArr[titleIndex]));
  }
  
  display.display();
}


void setup(){
  softwarei2c.begin(EEPROM_SDA, EEPROM_SCL);
  loadEEPROM();
  initRotary();
  initDisplay();

  if (DEBUG_ENABLED){
    Serial.begin(9600);
  }

  enableInterrupts();
}

void loop(){
  if (displayUpdateIterCou < 2){
    updateDisplay();
    displayUpdateIterCou++;
  }

  if(configWillBeSaved){
    disableInterrupts();
    saveConfig();
    configWillBeSaved = false;
    enableInterrupts();
  }

 
  //Serial.println(readEEPROM(0));
}

void loadEEPROM(){
  for(int i = 0; i < (MENU_COUNT - 1); i++){
    varArr[i] = readEEPROM(i);
    varArr_x2[i] = 2*readEEPROM(i);
  }  
}

void dumpEEPROM(){
  for(int i = 0; i < (MENU_COUNT - 1); i++){
    updateEEPROM(i, (byte) varArr[i]);
  } 
}

void updateEEPROM(int address, byte val){
    if (val != readEEPROM(address))
      writeEEPROM(address, val); 
}


// Function to write to EEPROOM
void writeEEPROM(int address, byte val)
{
  // Begin transmission to I2C EEPROM
  softwarei2c.beginTransmission(EEPROM_ADDR);

  if (EEPROM_TYPE == "24C32"){
    // Send memory address as two 8-bit bytes
    softwarei2c.write((int)(address >> 8));   // MSB
    softwarei2c.write((int)(address & 0xFF)); // LSB
  }
  else{
    softwarei2c.write((int)(address));   // Memory address is only one byte
  }
 
  // Send data to be stored
  softwarei2c.write(val);
 
  // End the transmission
  softwarei2c.endTransmission();
 
  // Add 5ms delay for EEPROM
  delay(5);
  if(DEBUG_ENABLED){
    Serial.print("Data written to address: ");
    Serial.println(address);
  }
}
 
// Function to read from EEPROM
byte readEEPROM(int address)
{
  // Define byte for received data
  byte rcvData = 0xFF;
 
  // Begin transmission to I2C EEPROM
  softwarei2c.beginTransmission(EEPROM_ADDR);
 
  
  if (EEPROM_TYPE == "24C32"){
    // Send memory address as two 8-bit bytes
    softwarei2c.write((int)(address >> 8));   // MSB
    softwarei2c.write((int)(address & 0xFF)); // LSB
  }

  else{
    softwarei2c.write((int)(address));   // Memory address is only one byte
  }
 
  // End the transmission
  softwarei2c.endTransmission();
 
  // Request one byte of data at current memory address
  softwarei2c.requestFrom(EEPROM_ADDR, 1);
 
  // Read the data and assign to variable
  rcvData =  softwarei2c.read();
 
  // Return the data as function output
  return rcvData;
}
 
