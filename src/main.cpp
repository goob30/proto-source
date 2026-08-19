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

// Matrix setup
#define MAX_DEVICES 7

#define CLK_PINR    18
#define DATA_PINR   23
#define CS_PINR     19

#define CLK_PINL    14
#define DATA_PINL   27
#define CS_PINL     13

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

MD_MAX72XX mxR = MD_MAX72XX(HARDWARE_TYPE, DATA_PINR, CLK_PINR, CS_PINR, MAX_DEVICES);
MD_MAX72XX mxL = MD_MAX72XX(HARDWARE_TYPE, DATA_PINL, CLK_PINL, CS_PINL, MAX_DEVICES);


#define LEFT_NOSE_OFFSET 0
#define LEFT_MOUTH_OFFSET 8
#define LEFT_EYE_OFFSET 40
#define RIGHT_EYE_OFFSET 0
#define RIGHT_MOUTH_OFFSET 16
#define RIGHT_NOSE_OFFSET 48

#define BTN_L0 32
#define BTN_L1 33
#define BTN_L2 25

int g_brightnessLevel = 0;

int globalCo2 = 30;
int globalFan = 100;
int globalHum = 30;

bool isTalking = true;
uint8_t mouthFrame = 0;
unsigned long lastMouthUpdate = 0;
const unsigned long TALK_INTERVAL = 80;

int blinkSequenceRegularEye[]  = {0, 1, 2, 1, 0};
int blinkSequenceConfusedEye[] = {0};
int *selectedBlinkSequence;
int numBlinkSeq;
int blindex = 0;

bool isBlinkEnabled = true;
bool isAnimatedBlinkEnabled = true;
unsigned long lastBlinkMillis = 0;
unsigned long nextBlinkInterval = (isTalking) ? random (3000, 4000) : random(6000, 8000);
unsigned long nextBlinkFrameTime = 20;
unsigned long lastBlinkFrameTime = 0;
int blinkDuration = (isAnimatedBlinkEnabled) ? nextBlinkFrameTime * numBlinkSeq : 50;

bool wasBlinking = false;



unsigned long previousMillis = 0;



void mxDrawBitmap(MD_MAX72XX& mx, const uint8_t* bmp, uint8_t width, uint8_t xOffset) {
  for (uint8_t x = 0; x < width; x++) {
    mx.setColumn(x + xOffset, bmp[x]);
  }
}


int currentEyeExpression = 0;

void updateBlinkSequence() {
  switch (currentEyeExpression) {
    case 0:
      selectedBlinkSequence = blinkSequenceRegularEye;
      numBlinkSeq = sizeof(blinkSequenceRegularEye) / sizeof(int);
      break;
    case 2:
      selectedBlinkSequence = blinkSequenceConfusedEye;
      numBlinkSeq = sizeof(blinkSequenceConfusedEye) / sizeof(int);
      break;
    default:
      selectedBlinkSequence = blinkSequenceRegularEye;
      numBlinkSeq = sizeof(blinkSequenceRegularEye) / sizeof(int);
      break;
  }
  blinkDuration = (isAnimatedBlinkEnabled) ? nextBlinkFrameTime * numBlinkSeq : 50;
}

bool isBlinking() {
  if ((millis() - lastBlinkMillis) > nextBlinkInterval) {
    lastBlinkMillis = millis();
    nextBlinkInterval = random(6000, 8000);
    return true;
  }
  return (millis() - lastBlinkMillis) < blinkDuration;
}



void renderFace() {
  mxL.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mxR.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  
  // Clear displays. duurhuhrhhhghughuhuhu
  mxL.clear();
  mxR.clear();

  mxDrawBitmap(mxL, noseL, 8, LEFT_NOSE_OFFSET);
  mxDrawBitmap(mxL, mouthFramesL[mouthFrame], 32, LEFT_MOUTH_OFFSET);
  
  int eyeidx;

  if (isBlinking()) {
    if (millis() - nextBlinkFrameTime > lastBlinkFrameTime) {
      lastBlinkFrameTime = millis();
      if (blindex < sizeof(blinkSequenceRegularEye) - 1) {
        blindex++;
      }
    }
    int frameidx = selectedBlinkSequence[blindex];
    mxDrawBitmap(mxL, eyeFramesClosingL[frameidx], 16, LEFT_EYE_OFFSET);
    mxDrawBitmap(mxR, eyeFramesClosingR[frameidx], 16, RIGHT_EYE_OFFSET);
  } else {
    blindex = 0;
    eyeidx = currentEyeExpression;
    mxDrawBitmap(mxL, eyeFramesL[eyeidx], 16, LEFT_EYE_OFFSET);
    mxDrawBitmap(mxR, eyeFramesR[eyeidx], 16, RIGHT_EYE_OFFSET);
  }

  

  mxDrawBitmap(mxR, mouthFramesR[mouthFrame], 32, RIGHT_MOUTH_OFFSET);
  mxDrawBitmap(mxR, noseR, 8, RIGHT_NOSE_OFFSET);

  
  mxL.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mxR.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  mxL.control(MD_MAX72XX::INTENSITY, g_brightnessLevel);
  mxR.control(MD_MAX72XX::INTENSITY, g_brightnessLevel);
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

  updateBlinkSequence();

  // Initialize matrices
  mxL.begin();
  mxR.begin();
  
  // Set brightness (
  mxL.control(MD_MAX72XX::INTENSITY, g_brightnessLevel);
  mxR.control(MD_MAX72XX::INTENSITY, g_brightnessLevel);
  
  renderFace();
  Serial.begin(115200);

  u8g2.begin();

  screenSwitch(Screen::HOME);
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

  screenSwitch(g_currentScreen);
  updateTalking();

  bool blinkingNow = isBlinking();


  if (isBlinking() || wasBlinking) {
    renderFace();
  }
  wasBlinking = blinkingNow;


  

  delay(10);
}