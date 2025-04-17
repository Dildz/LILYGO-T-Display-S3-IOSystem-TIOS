/*************************************************************
******************* INCLUDES & DEFINITIONS *******************
**************************************************************/

// Board libraries
#include <Arduino.h>     // core Arduino library
#include <EEPROM.h>      // for EEPROM storage
#include <TFT_eSPI.h>    // for TFT display control
#include <driver/adc.h>  // for ADC control
#include "esp_adc_cal.h" // for ADC calibration

// Font header file
#include "NotoSansBold15.h"

/* 
Create display and sprite objects:
 - lcd: Main display object
 - sprite: Primary drawing surface
*/
TFT_eSPI lcd = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&lcd);

#define EEPROM_SIZE 48 // size of EEPROM storage

// Custom colours
#define blue 0x297F      // converted from #2d2dff
#define darkBlue 0x09CA  // converted from #083852
#define green 0x13E3     // converted from #107C1B
#define grey 0x3A08      // converted from #3B4143
#define lightBlue 0x8E7F // converted from #8CCDFF
#define offWhite 0xAD75  // converted from #ADADAD
#define orange 0xE320    // converted from #E36600
#define purple 0x38A8    // converted from #3A1442
#define seaGreen 0x1A8B  // converted from #164f57

// TFT_eSPI colours
#define tftBlack TFT_BLACK
#define tftRed TFT_RED
#define tftMagenta TFT_MAGENTA
#define tftWhite TFT_WHITE

// Colour arrays for different UI elements
unsigned short typeColours[5] = {orange, blue, green, purple, tftMagenta};
unsigned short pinColours[24] = {
  tftBlack, tftBlack, grey, grey, grey, grey, grey, grey, darkBlue, tftBlack, tftBlack, tftRed, tftRed,
  grey, grey, grey, grey, grey, grey, grey, darkBlue, darkBlue, tftBlack, tftRed
};
unsigned short stateColours[2] = {tftBlack, tftRed};

// Timer variables
unsigned long currentTime[2] = {0, 0};
unsigned long timerIntervals[2][2] = {{1000, 500}, {300, 300}};

// Pin configuration arrays
int pins[24] = {100, 100, 43, 44, 18, 17, 21, 16, 100, 100, 100, 100, 100, 1, 2, 3, 10, 11, 12, 13, 100, 100, 100, 100};
int pinStates[28] = {0};
int pinDebounce[28] = {0};

// Button debouncing
int debounce = 0;
static unsigned long lastLeftButtonTime = 0;
static unsigned long lastRightButtonTime = 0;
const unsigned long debounceInterval = 50; // ms

// Menu system variables
int menu = 0;
int item = 0;
int selection = 0;
int pwmChannel = 0;
int menuTextX, menuTextY, selectorX, selectorY;
int selectedTimerIndex = 3;
int timerStateSelection = 3;

// Menu pin mapping arrays
int menuPins[18] = {50, 2, 3, 4, 5, 6, 7, 13, 14, 15, 16, 17, 18, 19, 24, 25, 26, 27};
int menuPins2[18] = {0};
int menuPins3[18] = {0};
int menuPins4[18] = {0};
int menuItems[10] = {5, 14, 7, 7, 3, 3, 4, 5, 5, 5};

// UI position variables
int pinBoxX, pinBoxY, lineStartX, lineEndX, stateCircleX, sourceLabelX, valueDisplayX;

// UI layout constants
byte fromTop = 52;
byte fromLeft = 58;
byte width = 24;
byte height = 17;

// Pin type configuration (0 = not set, 1 = inp, 2 = pullup, 3 = out, 4 = ana, 5 = pwm, 6)
byte pinTypes[28] = {0, 0, 0, 0, 2, 4, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 3, 5, 0, 0, 0, 0, 1, 2, 6, 6};
byte pinSources[28] = {100, 100, 100, 100, 100, 100, 100, 124, 100, 100, 100, 100, 100, 100, 100, 100, 25, 100, 26, 5, 100, 100, 100, 100, 100, 100};

// Timer configuration
byte timerBaseValues[4] = {250, 250, 150, 150}; 
byte timerMultipliers[4] = {10, 10, 10, 10};
byte timerSources[4] = {0};

// Button state tracking
bool pinButtonPressed[28] = {0};
bool waitForButtonRelease = false;
static bool leftButtonPressed = false;
static bool rightButtonPressed = false;
bool uiMode = 0;     // 0 = run mode, 1 = menu mode
bool menuAction = 0; // prevents multiple menu actions

// Other variables
unsigned long startTime = 0;       // device startup time
unsigned long lastFrameTime = 0;   // for FPS calculation
float fps = 0;                     // current FPS value
char uptimeString[9] = "00:00:00"; // HH:MM:SS uptime format
unsigned long lastPowerRead = 0; // for battery monitoring
const unsigned long powerReadInterval = 5000; // 5sec
int millivolts = 0;        // in mV
float supplyVoltage = 0.0; // in V

// Pin label strings
String pinLabels1[28] = {
  "G", "G", "43", "44", "18", "17", "21", "16", "NC", "G", "G", "3V", "3V", "1", "2", "3", "10", "11", "12", "13", "NC", "NC", "G", "5V", "0", "14", "T1", "T2"
};
String pinLabels2[28] = {
  "G", "G", "43", "44", "18", "17", "21", "16", "NC", "G", "G", "3V", "3V", "1", "2", "3", "10", "11", "12", "13", "NC", "NC", "G", "5V", "PB1", "PB2", "T1", "T2"
};
String pinTypeLabels[5] = {"INP", "SW", "OUT", "ANA", "PWM"};

// Menu system strings
String menuTitles[10] = {
  "MENU", "SELECT PIN", "SELECT TYPE", "SET SOURCE", "PWM", "SET TIMERS", "", "", "MULTIPLIER", "BRIGHTNESS"
};
String firstMenu[10][28] = {
  {"EXIT", "Reset All", "Set Pin", "Set Timer", "Brightness", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""}, 
  {"BACK", "43", "44", "18", "17", "21", "16", "1", "2", "3", "10", "11", "12", "13", "PB1", "PB2", "T1", "T2", "", "", "", "", "", "", "", "", "", ""},
  {"BACK", "NOT SET", "INP_PULLUP", "ON/OFF SW", "OUTPUT", "ANALOG ", "PWM", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""},
  {"HIGH", "LOW", "T1", "!T1", "T2", "!T2", "PB1", "!PB1", "PB2", "!PB", "9", "9", "9", "9", "9", "9", "9", "10", "", "", "", "", "", "", "", "", "", ""},
  {"50", "100", "150"},
  {"BACK", "SET T1", "SET T2"},
  {"BACK", "ON TIME", "OFF TIME", "MULTIPLIER"},
  {"1", "50", "100", "150", "250"},
  {"1", "10", "100", "200", "250"},
  {"50", "100", "150", "200", "250"}
};


/*************************************************************
********************** HELPER FUNCTIONS **********************
**************************************************************/

// Function to read pin configurations from EEPROM
void readEprom() {
  for(int i=0; i<24; i++) {
    pinTypes[i] = EEPROM.read(i);

    if(pinTypes[i]>5) {
      pinTypes[i] = 0; // reset invalid types
    }
  }

  for(int j=0; j<24; j++) {
    pinSources[j] = EEPROM.read(j+24);
  }
}

// Function to write pin configurations to EEPROM
void writeEprom() {
  for(int i=0; i<24; i++) {
    EEPROM.write(i, pinTypes[i]);
    EEPROM.commit();
  }

  for(int j=0; j<24; j++) {
    EEPROM.write(j+24, pinSources[j]);
    EEPROM.commit();
  }
}

// Function to detach a pin and reset it's configuration
void detach(int pin) {
  for(int i=0; i<24; i++) {
    if(pinSources[i] == pin) {
      pinTypes[i] = 0;
      pinSources[i] = 100;
      pinMode(pin, OUTPUT);
      digitalWrite(pin, 0);
    }
  }
}

// Function to initialize pins based on their configured types
void setupPins() {
  int nextPwmChannel = 1;

  for(int i=0; i<24; i++) {
    if(pinTypes[i] == 1 || pinTypes[i] == 2) { // input pullup or switch
      pinMode(pinLabels1[i].toInt(), INPUT_PULLUP);
    }

    if(pinTypes[i] == 3) { // output
      pinMode(pinLabels1[i].toInt(), OUTPUT);
    }

    if(pinTypes[i] == 5) { // PWM
      ledcSetup(nextPwmChannel, 5000, 8);
      ledcAttachPin(pinLabels1[i].toInt(), nextPwmChannel); 
      nextPwmChannel++;
    }
  }
}

// Function to read and process all pin states
void readPins() {
  static int smoothedValues[28] = {0}; // smoothed analog values
  const float smoothingFactor = 0.1;   // adjust as needed (range: 0.0 to 1.0)
  static unsigned long lastModeToggleTime = 0;
  pwmChannel = 1;

  // Update timer base values if sources are set
  for(int i=0; i<4; i++) {
    if(timerSources[i] != 0) {
      timerBaseValues[i] = pinStates[timerSources[i]];
    }
  }

  // Calculate timer intervals
  timerIntervals[0][0] = timerBaseValues[0] * timerMultipliers[0];
  timerIntervals[0][1] = timerBaseValues[1] * timerMultipliers[1];
  timerIntervals[1][0] = timerBaseValues[2] * timerMultipliers[2];
  timerIntervals[1][1] = timerBaseValues[3] * timerMultipliers[3];

  // Check for UI mode toggle (both buttons pressed)
  if(digitalRead(0) == 0 && digitalRead(14) == 0) {
    if(debounce == 0 && millis() - lastModeToggleTime > 200) {
        debounce = 1; 
        uiMode = !uiMode;
        waitForButtonRelease = true;
        lastModeToggleTime = millis();
    }
  } else {
    // Only reset debounce when both buttons are released
    if(digitalRead(0) == 1 && digitalRead(14) == 1) {
        debounce = 0;
    }
  }

  // Read and process each pin
  for(int i=0; i<26; i++) {
    if(pinTypes[i] == 1) { // if pin is input pullup
      pinStates[i] = digitalRead(pinLabels1[i].toInt());
    }

    if(pinTypes[i] == 2) { // if pin is switch
      if(digitalRead(pinLabels1[i].toInt()) == 0) {
        if(pinDebounce[i] == 0) {
          pinDebounce[i] = 1;
          pinButtonPressed[i] =! pinButtonPressed[i];
          pinStates[i] = pinButtonPressed[i];
        }
      }
      else {
        pinDebounce[i] = 0;
      }
    }

    if(pinTypes[i] == 3 && pinSources[i] != 100) {   // output with source
      if(pinSources[i] > 100 && pinSources[i] < 200) { // inverted source
        pinStates[i] =! pinStates[pinSources[i] - 100];
      }

      if(pinSources[i] < 100) { // direct source
        pinStates[i] = pinStates[pinSources[i]];
      }

      if(pinSources[i] >= 200) { // fixed value
        pinStates[i] = pinSources[i] - 200;
        digitalWrite(pinLabels1[i].toInt(),pinStates[i]);
      }
    }

    if(pinTypes[i] == 4) { // analog input
      int rawValue = analogRead(pinLabels1[i].toInt());
      smoothedValues[i] = smoothedValues[i] * (1.0 - smoothingFactor) + rawValue * smoothingFactor;
      pinStates[i] = map(smoothedValues[i], 0, 4095, 0, 255);
    }

    if(pinTypes[i] == 5) { // PWM output
      if(pinSources[i] > 100) { // fixed value
        pinStates[i] = pinSources[i] - 100;
      }
      else { // source value
        pinStates[i] = pinStates[pinSources[i]];
      }
      ledcWrite(pwmChannel, pinStates[i]);
      pwmChannel++;
    }
  }
}

// Function to reset all pin configurations
void clearPins() {
  for(int i=0; i<24; i++) {
    pinTypes[i] = 0;
    pinSources[i] = 100;

    if(pins[i] != 100) {
      pinMode(pins[i], OUTPUT);
      digitalWrite(pins[i], 0);
    }
  }
}

// Function to find all input pins for menu system
void findInputs() {
  int n = 10;

  for(int i=0; i<24; i++) {
    if(pinTypes[i] == 1 || pinTypes[i] == 2) {
      menuPins2[n] = i;
      firstMenu[3][n] = pinLabels1[i];
      n++;
      menuPins2[n] = i;
      firstMenu[3][n] = "!" + pinLabels1[i];
      n++;
    }
  }
  menuItems[3] = n;
}

// Function to find all analog pins for menu system
void findAnalogs() {
  int m = 3;
  menuItems[4] = 3;

  for(int i = 0; i < 24; i++) {
    if(pinTypes[i] == 4) {
      firstMenu[4][m] = "PIN " + pinLabels1[i];
      menuPins3[m] = i;
      m++;
    }
  }
  menuItems[4] = m;
}

// Function to find all analog pins for timer menu
void findAnalogs2() {
  int m = 5;
  menuItems[7] = m;

  for(int i = 0; i < 24; i++) {
    if(pinTypes[i] == 4) {
      firstMenu[7][m] = "PIN " + pinLabels1[i];
      menuPins4[m] = i;
      m++;
    }
  }
  menuItems[7] = m;
}

// Function to draw and handle the menu system
void setPins() {
  static unsigned long lastActionTime = 0;
  menuAction = 0;
  sprite.fillSprite(tftBlack);

  // Draw menu background
  sprite.fillSmoothRoundRect(99, 24, 62, 244, 6, offWhite, tftBlack);
  sprite.fillSmoothRoundRect(4, 296, 40, 20, 4, darkBlue, tftBlack);
  sprite.fillSmoothRoundRect(126, 296, 40, 20, 4, darkBlue, tftBlack);
  sprite.drawLine(4, 20, 92, 20, grey);
  sprite.fillRect(4, 24, 86, 244, purple);
  sprite.drawLine(4, 270, 92, 270, grey);
  sprite.drawLine(92, 20, 92, 270, grey);
  sprite.setTextDatum(4);

  // Draw pin type legend
  for(int i=0; i<5; i++) {
    sprite.fillSmoothRoundRect(4+i*32, 278, 30, 12, 2, typeColours[i], tftBlack);
    sprite.setTextColor(tftWhite, typeColours[i]);
    sprite.drawString(pinTypeLabels[i], 4+i*32+30/2, 278+7);
  }

  // Draw all pins in menu mode
  for(int i=0; i<24; i++) {
    if(i<12) {
      pinBoxX = 102;
      pinBoxY = 28 + i * 20;
      lineStartX = 4;
      lineEndX = pinBoxX;
      stateCircleX = 46;
      sourceLabelX = 30;
      valueDisplayX = sourceLabelX - 8;
    }
    else {
      pinBoxX = 102 + 30;
      pinBoxY = 28+(i-12) * 20;
      lineStartX = 154, lineEndX = pinBoxX + width;
      stateCircleX = pinBoxX + width + 10;
      sourceLabelX = stateCircleX + 18;
      valueDisplayX = sourceLabelX - 10;
    }
    
    // Draw pin box with appropriate colour
    if(pinTypes[i]!=0) {
      sprite.fillSmoothRoundRect(pinBoxX, pinBoxY, width, height, 2, typeColours[pinTypes[i]-1], offWhite);
      sprite.setTextColor(tftWhite, typeColours[pinTypes[i]-1]);
      sprite.drawString(pinLabels1[i], pinBoxX+width/2, pinBoxY+height/2, 2);
    }
    else {
      sprite.fillSmoothRoundRect(pinBoxX, pinBoxY, width, height, 2, pinColours[i], offWhite);
      sprite.setTextColor(tftWhite, pinColours[i]);
      sprite.drawString(pinLabels1[i], pinBoxX+width/2, pinBoxY+height/2, 2);
    }

    // Highlight selected pin
    if(pinLabels1[i]==firstMenu[1][selection]) {
      sprite.drawRoundRect(pinBoxX, pinBoxY, width, height, 2, tftRed);
      sprite.drawRoundRect(pinBoxX-1, pinBoxY-1, width+2, height+2, 2, tftRed);
    }
  }

  // Draw menu title and buttons
  sprite.setTextDatum(0);
  sprite.loadFont(NotoSansBold15);
  sprite.setTextColor(tftWhite, tftBlack);
  sprite.drawString(menuTitles[menu], 4, 4, 2);
  sprite.drawString("SEL", 12, 299, 2);
  sprite.drawString("OK", 135, 299, 2);
  sprite.unloadFont();
  
  // Draw menu footer
  sprite.setTextDatum(4);
  sprite.setTextColor(tftWhite, tftBlack);
  sprite.drawString("TIOS", 85, 302);
  sprite.drawString("SETUP", 85, 312);

  // Draw menu items
  sprite.setTextDatum(0);
  sprite.setTextColor(lightBlue, purple);
  
  for(int i=0; i<menuItems[menu]; i++) {
    if(i<16) {
      menuTextX = 14;
      menuTextY = 24;
    }
    else {
      menuTextX = 62;
      menuTextY = 24 - 240;
    }
    sprite.drawString(firstMenu[menu][i], menuTextX, menuTextY+(i*15), 2);
  }

  // Draw menu selector
  if(item<16) {
    selectorX = 7;
    selectorY = 32;
  }
  else {
    selectorX = 54;
    selectorY = 32 - 240;
  }

  sprite.fillCircle(selectorX, selectorY+(item*15), 3, orange);
  
  // Handle left button (navigation) - only if not waiting for release
  if(digitalRead(0)==0) {
    if(!leftButtonPressed && !waitForButtonRelease && millis() - lastLeftButtonTime > debounceInterval) {
      leftButtonPressed = true;
      lastLeftButtonTime = millis();
      
      item++;
      if(item == menuItems[menu]) {
        item = 0;
      }
      if(menu==1 && item>0) {
        selection = item;
      }
    }
  } else {
    leftButtonPressed = false;
    // Clear wait flag if both buttons are released
    if(waitForButtonRelease && digitalRead(14)==1) {
      waitForButtonRelease = false;
    }
  }

  // Handle right button (selection) - only if not waiting for release
  if(digitalRead(14)==0) {
    if(!rightButtonPressed && !waitForButtonRelease && millis() - lastRightButtonTime > debounceInterval) {
      rightButtonPressed = true;
      lastRightButtonTime = millis();
      
      // Main menu actions
      if(menu==0 && item==0) { // EXIT 
        uiMode = 0;
        writeEprom();
        setupPins();
      }

      if(menu==0 && item==1) { // clear all pins
        clearPins();
      }

      if(menu==0 && item==2 && menuAction==0) { // set Pin
        menu = 1;
        item = 0;
        menuAction = 1;
      }

      if(menu==0 && item==4 && menuAction==0) { // brightness
        menu = 9;
        item = 0;
        menuAction = 1;
      }
      
      if(menu==1 && item==0 && menuAction==0) { // BACK
        menu = 0;
        item = 0;
        menuAction = 1;
      }

      if(menu==1 && item!=0 && menuAction==0) { // pin selected
        menu = 2;
        item = 0;
        menuAction = 1;
      }

      if(menu==2 && item==0 && menuAction==0) { // BACK
        menu = 1;
        item = 0;
        menuAction = 1;
      }

      if(menu==2 && item==1 && menuAction==0) { // not set
        detach(menuPins[selection]);
        menu = 0;
        item = 0;
        menuAction = 1;
        pinTypes[menuPins[selection]] = 0;
        pinSources[menuPins[selection]] = 100;
      }

      if(menu==2 && item==2 && menuAction==0) { // input pullup
        detach(menuPins[selection]);
        menu = 0;
        item = 0;
        menuAction = 1;
        pinTypes[menuPins[selection]] = 1;
        pinSources[menuPins[selection]] = 100; 
        pinMode(firstMenu[1][selection].toInt(), INPUT_PULLUP);
      }

      if(menu==2 && item==3 && menuAction==0) { // switch
        detach(menuPins[selection]);
        pinStates[menuPins[selection]] = 0;
        menu = 0;
        item = 0;
        menuAction = 1;
        pinTypes[menuPins[selection]] = 2;
        pinSources[menuPins[selection]] = 100;
        pinMode(firstMenu[1][selection].toInt(), INPUT_PULLUP);
      }

      if(menu==2 && item==5 && menuAction==0) { // analog
        detach(menuPins[selection]);
        menu = 0;
        item = 0;
        menuAction = 1;
        pinTypes[menuPins[selection]] = 4;
        pinSources[menuPins[selection]] = 100;
      }

      if(menu==2 && item==4 && menuAction==0) { // output
        detach(menuPins[selection]);
        findInputs();
        menu = 3;
        item = 0;
        menuAction = 1;
        pinTypes[menuPins[selection]] = 3;
        pinSources[menuPins[selection]] = 100;
        pinMode(firstMenu[1][selection].toInt(), OUTPUT);
      }

      // Output menu items
      if(menu==3 && item==0 && menuAction==0) { // HIGH
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 201;
      }

      if(menu==3 && item==1 && menuAction==0) { // LOW
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 200;
      }

      if(menu==3 && item==2 && menuAction==0) { // T1
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 26;
      }

      if(menu==3 && item==3 && menuAction==0) { // !T1
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 126;
      }

      if(menu==3 && item==4 && menuAction==0) { // T2
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 27;
      }

      if(menu==3 && item==5 && menuAction==0) { // !T2
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 127;
      }

      if(menu==3 && item==6 && menuAction==0) { // PB1
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 24;
      }
      
      if(menu==3 && item==7 && menuAction==0) { // !PB1
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 124;
      }

      if(menu==3 && item==8 && menuAction==0) { // PB2
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 25;
      }

      if(menu==3 && item==9 && menuAction==0) { // !PB2
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 125;
      }

      //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
      if(menu==3 && item>9 && menuAction==0) { // other pins
        menu = 0;
        menuAction = 1;
      
        if(firstMenu[3][item].charAt(0)=='!') {
          pinSources[menuPins[selection]] = menuPins2[item] + 100;
          item = 0;
        }
        else {
          pinSources[menuPins[selection]] = menuPins2[item];
          item = 0;
        }
      }
      //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

      // PWM value selection menu
      if(menu==2 && item==6 && menuAction==0) { // output
        detach(menuPins[selection]);
        findAnalogs();
        menu = 4;
        item = 0;
        menuAction = 1;
        pinTypes[menuPins[selection]] = 5;
        pinSources[menuPins[selection]] = 100;
      }

      if(menu==4 && item==0 && menuAction==0) {
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 150;
      }

      if(menu==4 && item==1 && menuAction==0) {
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 200;
      }

      if(menu==4 && item==2 && menuAction==0) {
        menu = 0;
        item = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = 250;
      }

      if(menu==4 && item>2 && menuAction==0) { // analog pins as source
        menu = 0;
        menuAction = 1;
        pinSources[menuPins[selection]] = menuPins3[item];
        item = 0;
      }

      //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

      // Timer selection menu
      if(menu==0 && item==3 && menuAction==0) {
        menu = 5;
        item = 0;
        menuAction = 1;
      }

      if(menu==5 && item==0 && menuAction==0) { // BACK
        menu = 0;
        item = 0;
        menuAction = 1;
      }

      if(menu==5 && item==1 && menuAction==0) { // set T1
        menu = 6;
        selectedTimerIndex=0;
        menuTitles[6] = "Set T1";
        item = 0;
        menuAction = 1;
      }

      if(menu==5 && item==2 && menuAction==0) { // set T2
        menu = 6;
        selectedTimerIndex=1;
        menuTitles[6] = "Set T2";
        item = 0;
        menuAction = 1;
      }

      // Timer configuration menu
      if(menu==6 && item==0 && menuAction==0) { // BACK
        menu = 0;
        item = 0;
        menuAction = 1;
      }

      if(menu==6 && item==1 && menuAction==0) { // timer ON
        findAnalogs2();
        menu = 7;
        menuTitles[7] = "ON TIME";
        item = 0;
        menuAction = 1;
        timerStateSelection=0;
      } 
        
      if(menu==6 && item==2 && menuAction==0) { // timer OFF
        findAnalogs2();
        menu = 7;
        menuTitles[7] = "OFF TIME";
        item = 0;
        menuAction = 1;
        timerStateSelection=1;
      } 
        
      if(menu==6 && item==3 && menuAction==0) { // timer multiplier
        menu = 8;
        item = 0;
        menuAction = 1;
      } 

      // Timer interval selection menu
      if(menu==7 && menuAction==0 && item<5) { // fixed values
        timerBaseValues[(selectedTimerIndex*2) + timerStateSelection] = firstMenu[7][item].toInt();
        item = 0;
        menuAction = 1;
        menu = 6;
        timerSources[(selectedTimerIndex*2) + timerStateSelection] = 0;
      } 

      if(menu==7 && menuAction==0 && item>4) { // analog pins as source
        timerSources[(selectedTimerIndex*2) + timerStateSelection] = menuPins4[item];
        item = 0;
        menuAction = 1;
        menu = 6;
      } 

      // Timer multiplier selection menu
      if(menu==8 && menuAction==0) {
        timerMultipliers[selectedTimerIndex*2] = firstMenu[8][item].toInt();
        timerMultipliers[selectedTimerIndex*2+1] = firstMenu[8][item].toInt(); 
        item = 0;
        menuAction = 1;
        menu = 6;
      } 
      //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

      // Brightness selection menu
      if(menu==9 && menuAction==0) {
        ledcWrite(0, firstMenu[9][item].toInt());
        item = 0;
        menu = 0;
      }

      if (millis() - lastActionTime > 250) {
          lastActionTime = millis();
          // Continue with action
      }
    }
  }
  else {
    rightButtonPressed = false;
    // Clear wait flag if both buttons are released
    if(waitForButtonRelease && digitalRead(0)==1) {
      waitForButtonRelease = false;
    }
  }
  sprite.pushSprite(0, 0);
}

// Function to read supply voltage
void readSupplyVoltage() {
  uint32_t rawValue = analogRead(4); // GPIO4
  
  // Calculate with floating point precision first
  float calculated_mV = (rawValue * 2 * 3.3 * 1000) / 4096.0;
  
  // Store raw millivolts as integer
  millivolts = static_cast<int>(calculated_mV);
  
  // Calculate supply voltage
  supplyVoltage = calculated_mV / 1000.0f;
}

// Function to calculate device uptime
void calculateUptime() {
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  
  byte hours = seconds / 3600;
  byte minutes = (seconds % 3600) / 60;
  byte secs = seconds % 60;
  
  sprintf(uptimeString, "%1d:%02d:%02d", hours, minutes, secs); // remove leading zeros
}

// Function to calculate the FPS
void calculateFPS() {
  static unsigned long lastCalcTime = 0;
  static int frameCount = 0;
  
  unsigned long currentTime = millis();
  frameCount++;
  
  // Calculate FPS every second
  if (currentTime - lastCalcTime >= 1000) {
    fps = frameCount * 1000.0 / (currentTime - lastCalcTime);
    frameCount = 0;
    lastCalcTime = currentTime;
  }
}

// Function to draw the display
void drawDisplay() {
  sprite.fillSprite(offWhite);

  // Draw timer 1 UI elements
  sprite.fillSmoothRoundRect(4, 20, 78, 28, 4, seaGreen, offWhite);
  sprite.fillSmoothRoundRect(4, 30, 50, 38, 4, seaGreen, offWhite);
  sprite.fillSmoothRoundRect(5, 21, 76, 26, 4, tftWhite, seaGreen);
  sprite.fillSmoothRoundRect(5, 30, 48, 37, 4, tftWhite, seaGreen);
  sprite.fillSmoothCircle(40, 55, 9, seaGreen);
  sprite.setTextDatum(4);
  
  // Draw timer 1 labels
  sprite.setTextDatum(0);
  sprite.setTextColor(seaGreen, tftWhite);
  sprite.drawString("ON " + String(timerIntervals[0][0]), 8, 25);
  sprite.drawString("OFF " + String(timerIntervals[0][1]), 8, 35);

  // Draw timer 2 UI elements
  sprite.fillSmoothRoundRect(90, 20, 76, 28, 4, seaGreen, offWhite);
  sprite.fillSmoothRoundRect(118, 30, 48, 38, 4, seaGreen, offWhite);
  sprite.fillSmoothRoundRect(91, 21, 74, 26, 4, tftWhite, seaGreen);
  sprite.fillSmoothRoundRect(119, 30, 46, 37, 4, tftWhite, seaGreen);
  sprite.fillSmoothCircle(132, 55, 10, seaGreen);
  sprite.setTextDatum(4);

  // Draw timer 2 labels
  sprite.setTextColor(tftWhite, seaGreen);
  sprite.setTextDatum(0);
  sprite.setTextColor(seaGreen, tftWhite);
  sprite.drawString("ON " + String(timerIntervals[1][0]), 94, 25);
  sprite.drawString("OFF " + String(timerIntervals[1][1]), 94, 35);
  sprite.setTextDatum(4);
  
  // Draw pin type legend
  for(int i=0; i<5; i++) {
    sprite.fillSmoothRoundRect(6+i*32, 4, 30, 12, 2, typeColours[i], offWhite);
    sprite.setTextColor(tftWhite, typeColours[i]);
    sprite.drawString(pinTypeLabels[i], 6+i*32+30/2, 4+6); 
  }

  // Draw all pins
  for(int i=0; i<24; i++) {
    // Calculate positions based on pin number
    if(i<12) {
      pinBoxX = fromLeft;
      pinBoxY = fromTop + i * 20;
      lineStartX = 4;
      lineEndX = pinBoxX;
      stateCircleX = 46;
      sourceLabelX = 30;
      valueDisplayX = sourceLabelX - 8;
    }
    else {
      pinBoxX = fromLeft + 32;
      pinBoxY = fromTop+(i-12)*20;
      lineStartX = 154, lineEndX = pinBoxX + width;
      stateCircleX = pinBoxX+width + 10;
      sourceLabelX = stateCircleX + 18;
      valueDisplayX = sourceLabelX - 20;
    }
    
    // Draw pin box
    sprite.fillSmoothRoundRect(pinBoxX, pinBoxY, width, height, 2, pinColours[i], offWhite);
    sprite.setTextColor(tftWhite, pinColours[i]);
    sprite.drawString(pinLabels1[i], pinBoxX+width/2, pinBoxY+height/2, 2);

    // Draw additional elements for configured pins
    if(pinTypes[i] != 0) {
      // Connection line
      sprite.drawLine(lineStartX, pinBoxY+height/2, lineEndX, pinBoxY+height/2, stateColours[pinStates[i]]);
      sprite.drawLine(lineStartX, pinBoxY+height/2+1, lineEndX, pinBoxY+height/2+1, stateColours[pinStates[i]]);

      // Pin type indicator
      sprite.fillSmoothRoundRect(lineStartX, pinBoxY+2, width-12, height-4, 2, typeColours[pinTypes[i] - 1]);
      sprite.setTextColor(tftWhite, typeColours[pinTypes[i] - 1]);
      sprite.drawString(pinTypeLabels[pinTypes[i] - 1].substring(0, 1), lineStartX+6, pinBoxY+3+((height-4) / 2));

      // Analog pin value display
      if(pinTypes[i] == 4) { // if analog pin
        sprite.fillSmoothRoundRect(valueDisplayX, pinBoxY+2, width+3, height-4, 4, typeColours[pinTypes[i] - 1]);
        sprite.setTextColor(tftWhite, typeColours[pinTypes[i] - 1]);
        sprite.drawString(String(pinStates[i]), valueDisplayX+14, 1+pinBoxY+height/2);
      }

      // PWM pin value display
      if(pinTypes[i] == 5) { // if PWM pin
        sprite.fillSmoothRoundRect(valueDisplayX, pinBoxY+2, width+3, height-4, 4, typeColours[pinTypes[i] - 1]);
        sprite.setTextColor(tftWhite, typeColours[pinTypes[i] - 1]);
        sprite.drawString(String(pinStates[i]), valueDisplayX+14, 1+pinBoxY+height/2);
      }

      // Output pin source label
      if(pinTypes[i] == 3) {
        sprite.setTextColor(grey, tftWhite);
        if(pinSources[i] > 100)
          sprite.drawString("!" + String(pinLabels2[pinSources[i] - 100]), sourceLabelX, pinBoxY+4);
        else
          sprite.drawString(String(pinLabels2[pinSources[i]]), sourceLabelX, pinBoxY+4);
      }
      
      // State indicator for non-PWM/Analog pins
      if(pinTypes[i]<4) {
        sprite.fillSmoothCircle(stateCircleX, pinBoxY+height/2, 5, stateColours[pinStates[i]], tftWhite);
        sprite.setTextColor(tftWhite, stateColours[pinStates[i]]);
        sprite.drawString(String(pinStates[i]), stateCircleX+1, 1+pinBoxY+height/2);
      }
    }
  }

  // Draw pushbutton indicators
  sprite.fillSmoothRoundRect(4, 294, 46, 22, 4, typeColours[pinTypes[24] - 1]);
  sprite.fillSmoothRoundRect(6,296,24,18,2,grey);
  sprite.fillSmoothRoundRect(120, 294, 46, 22, 4, typeColours[pinTypes[25] - 1]);
  sprite.fillSmoothRoundRect(122, 296, 24, 18, 2, grey);

  // Draw UI instructions
  sprite.setTextColor(tftWhite, grey);
  sprite.setTextColor(tftBlack, offWhite);
  sprite.drawString("Press both", 85, 300);
  sprite.drawString("for MENU", 85, 310);

  // Draw labels with custom font
  sprite.loadFont(NotoSansBold15);
  sprite.setTextColor(tftBlack, offWhite);
  sprite.drawString("PB1", 18, 288);
  sprite.drawString("PB2", 152, 288);
  sprite.drawString("T1", 18, 57);
  sprite.drawString("T2", 155, 57);

  // Draw timer values
  sprite.setTextColor(tftWhite, seaGreen);
  sprite.drawString(String(pinStates[26]), 40, 57, 2);
  sprite.drawString(String(pinStates[27]), 132, 57, 2);

  // Draw pushbutton labels and values
  sprite.setTextColor(tftWhite, grey);
  sprite.drawString(pinLabels1[24], 17, 306);
  sprite.drawString(pinLabels1[25], 134, 306);
  sprite.setTextColor(tftBlack, orange);
  sprite.drawString(String(pinStates[24]), 38, 306);
  sprite.setTextColor(tftWhite, typeColours[pinTypes[25] - 1]);
  sprite.drawString(String(pinStates[25]), 155, 306);

  // Draw project name info
  sprite.fillSmoothRoundRect(4, 212, 50, 58, 4, purple, offWhite);
  sprite.setTextDatum(0);
  sprite.setTextColor(tftWhite, purple);
  sprite.drawString("TIOS", 11, 214);
  sprite.unloadFont();
  sprite.drawString("T-Disp", 8, 229);
  sprite.drawString("Input", 8, 239);
  sprite.drawString("Output", 8, 249);
  sprite.drawString("System", 8, 259);

  // Device info
  sprite.fillSmoothRoundRect(117, 212, 50, 58, 4, purple, offWhite);
  sprite.loadFont(NotoSansBold15);
  sprite.setTextDatum(0);
  sprite.setTextColor(tftWhite, purple);
  sprite.drawString("INFO", 123, 214);
  sprite.unloadFont();
  sprite.drawString("Uptime:", 121, 229);
  sprite.drawString(uptimeString, 121, 239); // uptime value
  sprite.drawString("FPS:" + String(int(fps)), 121, 249); // FPS value
  sprite.drawString(String(getCpuFrequencyMhz()) + "MHz", 121, 259); // CPU frequency

  // Draw supply voltage
  sprite.setTextColor(tftBlack, offWhite);
  sprite.drawString("USB/BATT", 6, 72);
  sprite.drawString("PWR:" + String(supplyVoltage, 1) + "V", 6, 82); // supply voltage
  //sprite.drawString(String(supplyVoltage, 2) + "V", 12, 82);

  // Push the complete sprite to the display
  sprite.unloadFont();
  sprite.pushSprite(0, 0);
}


/*************************************************************
*********************** MAIN FUNCTIONS ***********************
**************************************************************/

// SETUP
void setup() {
  // Initialize buttons
  pinMode(0, INPUT_PULLUP);  // BOOT button GPIO0
  pinMode(14, INPUT_PULLUP); // KEY button GPIO14

  /* For battery power - GPIO15 must be set to HIGH,
   * otherwise nothing will be displayed when USB is not connected. */ 
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  // Initialize display
  lcd.init();
  sprite.createSprite(170, 320); // portrait
  
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Initialize backlight PWM
  ledcSetup(0, 10000, 8);
  ledcAttachPin(38, 0); // LCD backlight GPIO38
  ledcWrite(0, 160);
  
  // Load settings and setup pins
  readEprom();
  setupPins();

  // Initialize uptime & FPS counters
  startTime = millis();
  lastFrameTime = millis();

  // Take initial supply power reading
  readSupplyVoltage();
}

// MAIN LOOP
void loop() {
  // Handle timer 1
  if(pinStates[26] == 0) {
    if(millis() > currentTime[0] + timerIntervals[0][1]) {
      pinStates[26] = 1;
      currentTime[0] = millis();
    }
  }
  else {
    if(millis() > currentTime[0] + timerIntervals[0][0]) {
      pinStates[26] = 0;
      currentTime[0] = millis();
    }
  }

  // Handle timer 2
  if(pinStates[27] == 0) {
    if(millis() > currentTime[1] + timerIntervals[1][1]) {
      pinStates[27] = 1;
      currentTime[1] = millis();
    }
  }
  else {
    if(millis() > currentTime[1] + timerIntervals[1][0]) {
      pinStates[27] = 0;
      currentTime[1] = millis();
    }
  }

  // Operation mode
  if(uiMode == 0) {
    // Read supply voltage level periodically
    if (millis() - lastPowerRead >= powerReadInterval) {
      readSupplyVoltage(); // call supply voltage calculator
      lastPowerRead = millis();
    }

    calculateUptime(); // call uptime calculator
    calculateFPS();    // call FPS calculator
    readPins();        // read pin states
    drawDisplay();     // draw display
  }
  
  // Menu mode
  else {
    setPins(); // call menu system
  }
}
