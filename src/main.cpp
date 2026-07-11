#include <MD_MAX72xx.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <bitmaps.h>
#include <clamp.h>
#include <screens.h>

// Matrix setup
#define MAX_DEVICES 7

#define CLK_PINR    18
#define DATA_PINR   23
#define CS_PINR     19

#define CLK_PINL    14
#define DATA_PINL   27
#define CS_PINL     13

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

MD_MAX72XX mxR = MD_MAX72XX(HARDWARE_TYPE, DATA_PINR, CLK_PINR, CS_PINR, MAX_DEVICES);
MD_MAX72XX mxL = MD_MAX72XX(HARDWARE_TYPE, DATA_PINL, CLK_PINL, CS_PINL, MAX_DEVICES);


#define LEFT_NOSE_OFFSET 0
#define LEFT_MOUTH_OFFSET 8
#define LEFT_EYE_OFFSET 40
#define RIGHT_EYE_OFFSET 0
#define RIGHT_MOUTH_OFFSET 16
#define RIGHT_NOSE_OFFSET 48

#define BRIGHTNESS_LEVEL 1 


#define BTN_L0 32
#define BTN_L1 33
#define BTN_L2 25


int globalCo2 = 30;
int globalFan = 100;
int globalHum = 30;


bool isTalking = true;
uint8_t mouthFrame = 0;
unsigned long lastMouthUpdate = 0;
const unsigned long TALK_INTERVAL = 100;


void mxDrawBitmap(MD_MAX72XX& mx, const uint8_t* bmp, uint8_t width, uint8_t xOffset) {
  for (uint8_t x = 0; x < width; x++) {
    mx.setColumn(x + xOffset, bmp[x]);
  }
}


int currentExpression = 0;


void renderFace() {
  mxL.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mxR.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  
  // Clear displays. duurhuhrhhhghughuhuhu
  mxL.clear();
  mxR.clear();

  mxDrawBitmap(mxL, noseL, 8, LEFT_NOSE_OFFSET);
  mxDrawBitmap(mxL, mouthFramesL[mouthFrame], 32, LEFT_MOUTH_OFFSET);
  mxDrawBitmap(mxL, eyeFramesL[currentExpression], 16, LEFT_EYE_OFFSET);

  mxDrawBitmap(mxR, eyeFramesR[currentExpression], 16, RIGHT_EYE_OFFSET);
  mxDrawBitmap(mxR, mouthFramesR[mouthFrame], 32, RIGHT_MOUTH_OFFSET);
  mxDrawBitmap(mxR, noseR, 8, RIGHT_NOSE_OFFSET);
  
  
  mxL.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mxR.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}


void updateTalking() {
  if (!isTalking) return;

  if (millis() - lastMouthUpdate >= TALK_INTERVAL) {
    lastMouthUpdate = millis();
    mouthFrame = (mouthFrame + 1) % NUM_MOUTH_FRAMES;
    renderFace();
  }
}


bool lastL0 = HIGH;
bool lastL1 = HIGH;
bool lastL2 = HIGH;


void setup() {

  pinMode(BTN_L0, INPUT_PULLUP);
  pinMode(BTN_L1, INPUT_PULLUP);
  pinMode(BTN_L2, INPUT_PULLUP);

  // Initialize matrices
  mxL.begin();
  mxR.begin();
  
  // Set brightness (
  mxL.control(MD_MAX72XX::INTENSITY, BRIGHTNESS_LEVEL);
  mxR.control(MD_MAX72XX::INTENSITY, BRIGHTNESS_LEVEL);
  
  renderFace();
  Serial.begin(115200);

  u8g2.begin();

  screen_switch(Screen::HOME);
}


void loop() {
  bool curL0 = digitalRead(BTN_L0);
  bool curL1 = digitalRead(BTN_L1);
  bool curL2 = digitalRead(BTN_L2);

  if (lastL0 == HIGH && curL0 == LOW) {
    handleInput(BTN_L0, getMaxScreenIndex(g_currentScreen));
  }
  if (lastL1 == HIGH && curL1 == LOW) {
    handleInput(BTN_L1, getMaxScreenIndex(g_currentScreen));
  }
  if (lastL2 == HIGH && curL2 == LOW) {
    handleInput(BTN_L2, getMaxScreenIndex(g_currentScreen));
  }

  lastL0 = curL0;
  lastL1 = curL1;
  lastL2 = curL2;

  screen_switch(g_currentScreen);
  updateTalking();

  delay(10);
}