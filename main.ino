#define LCD_ROWS 4
#define LCD_COLS 20
#define X_LENGTH 42860
#define Y_LENGTH 150000


#include <EEPROM.h>
#include "src/Stepper_Shan/Stepper_Shan.h"
#include "src/Rotary_Shan/Rotary_Shan.h"
#include "src/LCD_I2C_Shan/LCD_I2C_Shan.h"

///////////////////////////////////////
#include <FastLED.h>

#define LED_PIN     4
#define NUM_LEDS    16
#define BRIGHTNESS  255
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
///////////////////////////////////////

/* Pin Declarations */
const uint8_t xLimit = A1, yLimit = A0;
uint8_t stirrer1Pin = 11, stirrer2Pin = 10;
uint8_t blueLightPin = A3;
Stepper_Shan stepperX(40, 9, 8); // enPin, cwPin, clkPin, activeLow=1
Stepper_Shan stepperY(50, 7, 6);
Stepper_Shan stepper360(60, 13, 12);

Rotary_Shan rotary(2, A2, 3); // aPin, bPin, swPin, activeLow=1
LCD_I2C_Shan lcd(0x27, LCD_COLS, LCD_ROWS); // lcdAdress, numCols, numRows

const byte leftArrow[] = {B00010, B00110, B01110, B11110, B11110, B01110, B00110, B00010};
const byte playingButton[] = {B11011, B11011, B11011, B11011, B11011, B11011, B11011, B11011};

/* Settings Variables */
uint8_t dipOrder[4];
unsigned int numCycles;
unsigned int dipSpeed;
unsigned long usPerDipStep;
unsigned int adTime, recTime, rinTime;
unsigned int dipLength;
unsigned int withdrawalSpeed;
unsigned int usPerWithdrawalStep;
byte stirrerOn[2];

const uint8_t numOfSettings = 11, numPages = 4;
const char* fields[numOfSettings] = {
  "Dip Order: ",
  "Num of Cycles: ",
  "Dip Speed(mm/s): ",
  "WD Speed(mm/s): ",
  "Dip Length: ",
  "AD Time(s): ",
  "REC Time(s): ",
  "Rin Time(s): ",
  "Stirrer 1: ",
  "Stirrer 2: ",
  "       < Run >      ",
};

const uint8_t valueColNums[][2] = {
  /* {colStart, colEnd} */
  {11, 14},
  {15, 17},
  {17, 18},
  {16, 17},
  {12, 14},
  {12, 14},
  {13, 15},
  {13, 15},
  {11, 13},
  {11, 13},
};

unsigned int temp;
uint8_t pageNum, prePageNum, arrowPosRow, settingsId;
unsigned long startMillis;



void setup() {
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
  pixelsRGB(0, 0, 0);

  pinMode(stirrer1Pin, OUTPUT);
  digitalWrite(stirrer1Pin, 0);
  pinMode(stirrer2Pin, OUTPUT);
  digitalWrite(stirrer2Pin, 0);
  pinMode(blueLightPin, OUTPUT);
  digitalWrite(blueLightPin, 1);

  lcd.init();
  lcd.createChar(0, leftArrow);
  lcd.createChar(1, playingButton);
  rotary.attachRotationInterrupt(rotationIsr);
  rotary.attachSwitchInterrupt(switchIsr);
  readCurrentSettings();
  lcd.clear();
  lcd.setPos(0, 2);
  lcd.print("SILAR Depositor");
  lcd.setPos(2, 2);
  lcd.print("Developed By:");
  lcd.setPos(3, 2);
  lcd.print("Energy Lab, CUET");
  delay(300);
}

void loop() {

  uint8_t mode_choice = EEPROM.read(22);
  if (mode_choice == 1) {
    stirrerMode();
  }

  lcd.clear();
  lcd.setPos(0, 2);
  lcd.print("1. Program Mode");
  lcd.setPos(1, 2);
  lcd.print("2. Stirrer Mode");

  lcd.setPos(0, 19);
  lcd.write(0);
  mode_choice = rotary.getValueInput(
  [](int value) {
    if (value == 0) {
      lcd.setPos(1, 19);
      lcd.write(' ');
      lcd.setPos(0, 19);
      lcd.write(0);
    } else {
      lcd.setPos(0, 19);
      lcd.write(' ');
      lcd.setPos(1, 19);
      lcd.write(0);
    }
  }, (int) 0, 0, 1);
  EEPROM.write(22, mode_choice);
  if (mode_choice == 0) {
    programMode();
  } else {
    stirrerMode();
  }
}


void programMode() {
  takeSettingsInput();

  /* Conversion from user input to Applied settings */
  /*
     mm/second to useconds/step conversion
     mm/seconds: 1 - 50
     steps/second = mm/seconds * 400
     useconds/step = 10^6 / steps/second
                   = 10^6 / (mm/seconds * 400)
                   = 2500 / mm/seconds
  */
  usPerDipStep = round(2500.0 / dipSpeed);
  usPerWithdrawalStep = round(2500.0 / withdrawalSpeed);


  lcd.clear();
  lcd.setPos(1, 0); lcd.print("  Initializing...");
  pixelsRGB(0, 0, 255);
  goInit();
  digitalWrite(stirrer1Pin, stirrerOn[0]);
  digitalWrite(stirrer2Pin, stirrerOn[1]);
  delay(1000);
  lcd.clear();
  lcd.setPos(0, 0); lcd.print("Order: ");
  for (int8_t i = 0; i < 4; ++i) {
    lcd.write('0' + dipOrder[i]);
  }
  lcd.setPos(1, 0); lcd.print("Cycle: ");
  int8_t currentPos = 0;
  lcd.setPos(2, 0); lcd.print("Position: ");
  lcd.setPos(3, 0); lcd.print("ETA: ");

  stepperY.stepsPerMicro(5000, usPerWithdrawalStep); // for not touching the limit switch

  lcd.setPos(3, 5); lcd.print("...");

  /* run the first cycle and measure the time */
  unsigned long tic = millis();
  lcd.setPos(1, 7); lcd.clearRange(7, 9); lcd.print(1); lcd.write('/'); lcd.print(numCycles);
  lcd.setPos(2, 10); lcd.clearRange(10, 14);
  for (int8_t j = 0; j < 4; ++j) {
    stepperY.stepsPerMicro(40000, usPerWithdrawalStep);
    handleStirrer(currentPos, 0);
    handleStirrer(dipOrder[j], 1);
//    Serial.print(currentPos); Serial.print(" "); Serial.println(dipOrder[j]);
    goFromTo(currentPos, dipOrder[j]);
    lcd.write('0' + dipOrder[j]);
    stepperY.stepsPerMicro(-40000, usPerDipStep);
    pixelsRGB(0, 255, 0);
    handleDipping(dipOrder[j]);
    pixelsRGB(0, 0, 255);
    currentPos = dipOrder[j];
  }
  unsigned long timeOneCycle = (millis() - tic) / 1000; // in seconds
  lcd.setPos(3, 5); lcd.clearRange(5, 19); lcd.print(seconds2Time(timeOneCycle * (numCycles - 1)));

  for (unsigned int i = 1; i < numCycles; ++i) {
    lcd.setPos(1, 7); lcd.clearRange(7, 9); lcd.print(i + 1); lcd.write('/'); lcd.print(numCycles);
    lcd.setPos(2, 10); lcd.clearRange(10, 14);
    for (uint8_t j = 0; j < 4; ++j) {
      stepperY.stepsPerMicro(40000, usPerWithdrawalStep);
      handleStirrer(currentPos, 0);
      handleStirrer(dipOrder[j], 1);
      goFromTo(currentPos, dipOrder[j]);
      lcd.write('0' + dipOrder[j]);
      stepperY.stepsPerMicro(-40000, usPerDipStep);
      pixelsRGB(0, 255, 0);
      handleDipping(dipOrder[j]);
      pixelsRGB(0, 0, 255);
      currentPos = dipOrder[j];
    }
    lcd.setPos(3, 5); lcd.clearRange(5, 19); lcd.print(seconds2Time(timeOneCycle * (numCycles - i - 1)));
  }

  lcd.clear();
  lcd.setPos(1, 0); lcd.print("   Task Completed");
  complete();
  settingsId = 0;
  pageNum = 1;
}


void stirrerMode() {
  uint8_t st1_speed = EEPROM.read(20);
  uint8_t st2_speed = EEPROM.read(21);
  analogWrite(stirrer1Pin, map(st1_speed, 0, 100, 0, 255));
  analogWrite(stirrer2Pin, map(st2_speed, 0, 100, 0, 255));
  lcd.clear();
  lcd.setPos(0, 0);
  lcd.print("Stirrer 1: ");
  lcd.print(st1_speed);
  lcd.setPos(0, 14);
  lcd.print("%");
  lcd.setPos(1, 0);
  lcd.print("Stirrer 2: ");
  lcd.print(st2_speed);
  lcd.setPos(1, 14);
  lcd.print("%");
  lcd.setPos(3, 0);
  lcd.print("        Exit");

  lcd.setPos(0, 19);
  lcd.write(0);
  uint8_t option = 0;
  while (option != 2)
  {
    option = rotary.getValueInput(
    [](int value) {
      lcd.setPos(0, 19);
      lcd.write(' ');
      lcd.setPos(1, 19);
      lcd.write(' ');
      lcd.setPos(3, 19);
      lcd.write(' ');
      switch (value) {
        case 0: lcd.setPos(0, 19); lcd.write(0); break;
        case 1: lcd.setPos(1, 19); lcd.write(0); break;
        case 2: lcd.setPos(3, 19); lcd.write(0); break;
      }
    }, (int)0, 0, 2);

    switch (option) {
      case 0:
        lcd.setPos(0, 19); lcd.write(1);
        st1_speed = rotary.getValueInput(
        [](int value) {
          lcd.setPos(0, 11);
          lcd.print("   ");
          lcd.setPos(0, 11);
          lcd.print(value);
        }, st1_speed, 0, 100);
        lcd.setPos(0, 19); lcd.write(0);
        EEPROM.write(20, st1_speed);
        analogWrite(stirrer1Pin, map(st1_speed, 0, 100, 0, 255));
        break;
      case 1:
        lcd.setPos(1, 19); lcd.write(1);
        st2_speed = rotary.getValueInput(
        [](int value) {
          lcd.setPos(1, 11);
          lcd.print("   ");
          lcd.setPos(1, 11);
          lcd.print(value);
        }, st2_speed, 0, 100);
        lcd.setPos(1, 19); lcd.write(0);
        EEPROM.write(21, st2_speed);
        analogWrite(stirrer2Pin, map(st2_speed, 0, 100, 0, 255));
        break;
      case 2: EEPROM.write(22, 0); analogWrite(stirrer1Pin, 0); analogWrite(stirrer2Pin, 0); break;
    }
  }
}
