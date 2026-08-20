// TODO: implement telemetry reading
//       move stuff into cpp
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

const int buttonPins[NUM_BUTTONS] = { BTN_INDEX_L, BTN_MIDDLE_L, BTN_INDEX_R, BTN_MIDDLE_R };
ButtonRole buttonRole[NUM_BUTTONS] = { ROLE_UP, ROLE_DOWN, ROLE_SELECT, ROLE_MISC };
bool buttonLastState[NUM_BUTTONS] = { HIGH, HIGH, HIGH, HIGH };

void setup() {
  /// initIfCrash();
  popup(initIfCrash(), true);
  setBreadcrumb("boot");

  for (int i = 0; i < NUM_BUTTONS; i++) pinMode(buttonPins[i], INPUT_PULLUP);

  updateBlinkSequence();

  initMatrices();

  renderFace();
  Serial.begin(115200);

  u8g2.begin();

  screenSwitch(Screen::HOME);
}

void loop() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    bool cur = digitalRead(buttonPins[i]);
    if (buttonLastState[i] == HIGH && cur == LOW) {
      handleInput(buttonRole[i], getMaxScreenIndex(g_currentScreen));
    }
    buttonLastState[i] = cur;
  }

  screenSwitch(g_currentScreen);
  updateTalking();

  bool blinkingNow = isBlinking();

  if (isBlinking() || wasBlinking) {
    renderFace();
  }
  wasBlinking = blinkingNow;

  delay(10);
}