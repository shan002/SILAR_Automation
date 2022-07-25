String seconds2Time(unsigned int seconds) {
  int minutes = seconds / 60;
  seconds = seconds % 60;
  int hours = minutes / 60;
  minutes = minutes % 60;
  return String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
}

void writeDefaultSettings() {
  for (int i = 0; i < 4; ++i) {
    EEPROM.write(i, i + 1);
  }

  eepromWriteInt(4, 100);		// numCycles
  eepromWriteInt(6, 100);		// dipSpeed
  eepromWriteInt(8, 100);		// withdrawalSpeed
  eepromWriteInt(10, 100);	// dipLength
  eepromWriteInt(12, 100);	// adTime
  eepromWriteInt(14, 100);  // recTime
  eepromWriteInt(16, 100);  // rinTime
  EEPROM.write(18, 0);			// stirrerOn[0]
  EEPROM.write(19, 0);			// stirrerOn[1]
  EEPROM.write(20, 0);      // st1_speed for stirrerMode
  EEPROM.write(21, 0);      // st2_speed for stirrerMode
  EEPROM.write(22, 0);      // mode_choice: programMode or stirrerMode
}

void readCurrentSettings() {
  /* Reading dip order from EEPROM */
  for (int i = 0; i < 4; ++i) {
    dipOrder[i] = EEPROM.read(i);
  }

  /* Reading other settings from EEPROM */
  numCycles       = eepromReadInt(4);
  dipSpeed        = eepromReadInt(6);
  withdrawalSpeed = eepromReadInt(8);
  dipLength       = eepromReadInt(10);
  adTime          = eepromReadInt(12);
  recTime          = eepromReadInt(14);
  rinTime          = eepromReadInt(16);
  stirrerOn[0]    = EEPROM.read(18);
  stirrerOn[1]    = EEPROM.read(19);
}

void rotationIsr() {
  rotary.handleRotationCount();
}

void switchIsr() {
  rotary.handleSwitchState();
}

void pixelsRGB(uint8_t r, uint8_t g , uint8_t b)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(r, g, b);
  }
  FastLED.show();
}
