int eepromReadInt(int address) {
  int ten = EEPROM.read(address + 1);
  int one = EEPROM.read(address);
  return (one & 0xFF) + ((ten & 0xFF) << 8);
}

void eepromWriteInt(int address, int value) {
  byte one = (value & 0xFF);
  byte ten = ((value >> 8) & 0xFF);
  EEPROM.write(address + 1, ten);
  EEPROM.write(address, one);
}

void showSettingsPage(uint8_t pageNum){
  lcd.clear();
  lcd.setPos(0, 13);
  lcd.print("Page: "); lcd.print(pageNum);
  uint8_t startRow = 3 * (pageNum-1);
  uint8_t endRow = 3 * pageNum;
  for(uint8_t i=startRow; i<endRow; ++i){
    lcd.setPos((i % 3) + 1, 0);
    if(i > 10){
      break;
    }
    lcd.print(String(fields[i]));
    lcd.clearRange(valueColNums[i][0], valueColNums[i][1]);
    switch(pageNum){
      case 1:
        switch(i){
          case 0:
            for (uint8_t j = 0; j < 4; ++j) {
              lcd.write('0' + dipOrder[j]);
            } break;
          case 1: lcd.print(numCycles); break;
          case 2: lcd.print(dipSpeed); break;
        }
        break;

      case 2:
        switch(i){
          case 3: lcd.print(withdrawalSpeed); break;
          case 4: lcd.print(dipLength); break;
          case 5: lcd.print(adTime); break;
        }
        break;

      case 3:
        switch(i){
          case 6: lcd.print(recTime); break;
          case 7: lcd.print(rinTime); break;
          case 8: lcd.print(stirrerOn[0] ? "ON" : "OFF"); break;
        }
        break;

      case 4:
        switch(i){
          case 9: lcd.print(stirrerOn[1] ? "ON" : "OFF"); break;
          case 10: break;
        }
        break;
    }
  }
}

void setArrowPos(uint8_t &arrowPosRow, uint8_t settingsId){
  uint8_t newRow = (settingsId % 3) + 1;
  if(arrowPosRow != newRow){
    lcd.setPos(arrowPosRow, 19);
    lcd.write(' ');
    lcd.setPos(newRow, 19);
    lcd.write(0);
    arrowPosRow = newRow;
  }
}

void changeArrowType(uint8_t rowNum, uint8_t &arrowType){
  arrowType = 1 - arrowType;
  lcd.setPos(rowNum, 19);
  lcd.write(' ');
  lcd.setPos(rowNum, 19);
  lcd.write(arrowType);
}

void takeSettingsInput(){
  showSettingsPage(1);
  lcd.setPos(1, 19); lcd.write(0);
  settingsId = 0;
  pageNum = 1;
  prePageNum = 1;
  arrowPosRow = 1;
  uint8_t arrowType = 0;

  while(settingsId != (numOfSettings-1)){
    settingsId = rotary.getValueInput(
      [](int id) {
        pageNum = (id/3) + 1;
        if(pageNum != prePageNum){
          showSettingsPage(pageNum);
          prePageNum = pageNum;
        }
        setArrowPos(arrowPosRow, id);
      }, settingsId, 0, numOfSettings-1);

    changeArrowType(arrowPosRow, arrowType);

    switch(settingsId){
      case 0:
        lcd.clearRange(valueColNums[settingsId][0], valueColNums[settingsId][1]);
        for(temp=0; temp<4; ++temp){
          lcd.setPos(1, valueColNums[0][0] + temp);
          lcd.write('0' + dipOrder[temp]);
          dipOrder[temp] = rotary.getValueInput(
            [](int value) {
              lcd.setPos(1, valueColNums[0][0] + temp);
              lcd.write('0' + value);
            },
            dipOrder[temp], 1, 4);
        }

        /* Write the dip order into EEPROM */
        for (int i = 0; i < 4; ++i) {
          EEPROM.write(i, dipOrder[i]);
        }

        break;

      case 8:
      case 9:
        stirrerOn[settingsId - 8] = rotary.getValueInput(
          [](int value) {
            lcd.clearRange(valueColNums[settingsId][0], valueColNums[settingsId][1]);
            lcd.print(value == 1 ? "ON" : "OFF");
          },
          stirrerOn[settingsId - 8], 0, 1);

        /**
         * Write the stirrerOn settings on EEPROM 
         * on address 16 --> stirrerOn[0]
         * on address 17 --> stirrerOn[1]
         **/
        EEPROM.write(10+settingsId, stirrerOn[settingsId-8]);

        break;

      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        unsigned int valueMin, valueMax;
        switch (settingsId) {
          case 1:  temp = numCycles;       valueMin = 1; valueMax = 999; break;
          case 2:  temp = dipSpeed;        valueMin = 1; valueMax = 40; break;
          case 3:  temp = withdrawalSpeed; valueMin = 1; valueMax = 40; break;
          case 4:  temp = dipLength;       valueMin = 1; valueMax = 999; break;
          case 5:  temp = adTime;          valueMin = 1; valueMax = 100; break;
          case 6:  temp = recTime;          valueMin = 2; valueMax = 100; break;
          case 7:  temp = rinTime;          valueMin = 1; valueMax = 100; break;
        }

        temp = rotary.getValueInput(
          [](int value) {
            lcd.clearRange(valueColNums[settingsId][0], valueColNums[settingsId][1]);
            lcd.print(value);
          },
          temp, valueMin, valueMax);

        switch (settingsId) {
          case 1:  numCycles       = temp; eepromWriteInt(4, temp); break;
          case 2:  dipSpeed        = temp; eepromWriteInt(6, temp); break;
          case 3:  withdrawalSpeed = temp; eepromWriteInt(8, temp); break;
          case 4:  dipLength       = temp; eepromWriteInt(10, temp); break;
          case 5:  adTime          = temp; eepromWriteInt(12, temp); break;
          case 6:  recTime          = temp; eepromWriteInt(14, temp); break;
          case 7:  rinTime          = temp; eepromWriteInt(16, temp); break;
        }
      break;

    }

    changeArrowType(arrowPosRow, arrowType);
  }
}
