void goInit() {
  /* Initialize in y direction */
  digitalWrite(7, 1); // stepperY.cwPin = 7 and 1 for up direction
  while (digitalRead(yLimit)) {
    stepperY.takeOneStep(100);
  }

  digitalWrite(7, 0); // stepperY.cwPin = 7 and 0 for down direction
  while (!digitalRead(yLimit)) {
    stepperY.takeOneStep(100);
  }
  
  stepperY.stepsPerMicro(-3200, 100); // For releasing the limit switch

  /* Initialize in x direction */
  digitalWrite(9, 0); // stepperX.cwPin = 5 and 0 for left direction
  while (digitalRead(xLimit)) {
    stepperX.takeOneStep(300);
  }

  /* Initialize in y direction */
  digitalWrite(7, 0); // stepperY.cwPin = 7 and 0 for down direction
  while (digitalRead(yLimit)) {
    stepperY.takeOneStep(100);
  }
}

void complete() {
  digitalWrite(stirrer1Pin, 0);
  digitalWrite(stirrer2Pin, 0);
  /* Initialize in y direction */
  digitalWrite(7, 1); // stepperY.cwPin = 7 and 1 for up direction
  while (digitalRead(yLimit)) {
    stepperY.takeOneStep(100);
  }

  digitalWrite(7, 0); // stepperY.cwPin = 7 and 0 for down direction
  while (!digitalRead(yLimit)) {
    stepperY.takeOneStep(100);
  }
  stepperY.stepsPerMicro(-3200, 100); // For releasing the limit switch

  /* Initialize in x direction */
  digitalWrite(9, 0); // stepperX.cwPin = 5 and 0 for left direction
  while (digitalRead(xLimit)) {
    stepperX.takeOneStep(300);
  }
}

void goFromTo(int8_t startPos, int8_t targetPos) {
  long inBetweenSteps = X_LENGTH / 5;
  stepperX.stepsPerChangingMicro((targetPos - startPos)*inBetweenSteps, 1000, 200);
}

void handleDipping(uint8_t currentPos) {
  if(currentPos == 1){
    delay(adTime*1000);
  }else if(currentPos == 3){
    delay(recTime*1000);
  }else if(currentPos == 2 || currentPos == 4){
//    int stepTime = (rinTime * 1000000 - 1000000) / 3200 / 2;
    int stepTime = 700;
    unsigned long tic = millis();
    while(1){
      stepper360.stepsPerMicro(3200, stepTime);
      if(millis()-tic > rinTime*1000){
        break;
      }
      stepper360.stepsPerMicro(-3200, stepTime);
      if(millis()-tic > rinTime*1000){
        break;
      }
    }
  }
}

void handleStirrer(uint8_t currentPos, uint8_t state) {
  if (state) {
    if (currentPos == 1) {
      digitalWrite(stirrer1Pin, 0);
    } else if (currentPos == 3) {
      digitalWrite(stirrer2Pin, 0);
    }
  } else {
    digitalWrite(stirrer1Pin, stirrerOn[0]);
    digitalWrite(stirrer2Pin, stirrerOn[1]);
  }
}
