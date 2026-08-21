// TODO: implement telemetry reading
//       make dead eye persist until closing crash prompt
//       unfuck the repo

#include <MD_MAX72xx.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include "bitmaps.h"
#include "clamp.h"
#include "screens.h"
#include "inputHandler.h"
#include "errorHandler.h"
#include "matrix.h"
#include "dataHandler.h"
#include "screenTypes.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void setup() {
  /// initIfCrash();
  const char* crashMsg = initIfCrash();
  if (crashMsg) {
    currentEyeExpression = 4;
    isBlinkEnabled = false;
  }
  popup(crashMsg, true);    
  setBreadcrumb("boot");

  initButtons();

  

  initMatrices();

  renderFace();
  updateLeds();
  Serial.begin(115200);

  u8g2.begin();

  screenSwitch(Screen::HOME);
}

void loop() {
  // for (int i = 0; i < NUM_BUTTONS; i++) {
  //   bool cur = digitalRead(buttonPins[i]);
  //   if (buttonLastState[i] == HIGH && cur == LOW) {
  //     handleInput(buttonRole[i], getMaxScreenIndex(g_currentScreen));
  //   }
  //   buttonLastState[i] = cur;
  // }

  pollInputs();

  screenSwitch(g_currentScreen);
  updateTalking();

  bool blinkingNow = isBlinking();
  if (blinkingNow || wasBlinking) {
      renderFace();
  }
  wasBlinking = blinkingNow;

  delay(10);
}