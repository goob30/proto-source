#include "bitmaps.h"
#include "matrix.h"
#include "dataHandler.h"
#include "timer.h"

bool isTalking = true;
uint8_t mouthFrame = 0;
unsigned long lastMouthUpdate = 0;

unsigned long previousMillis = 0;


int currentEyeExpression = 0;

int blinkSequenceRegularEye[5]  = {0, 1, 2, 1, 0};
int blinkSequenceConfusedEye[1] = {0};
int *selectedBlinkSequence;
int numBlinkSeq;
int blindex = 0;

const unsigned long TALK_INTERVAL = 80;

bool isBlinkEnabled = true;
bool isAnimatedBlinkEnabled = true;
unsigned long lastBlinkMillis = 0;
unsigned long nextBlinkInterval = (isTalking) ? random (3000, 4000) : random(6000, 8000);
unsigned long nextBlinkFrameTime = 20;
unsigned long lastBlinkFrameTime = 0;
int blinkDuration = (isAnimatedBlinkEnabled) ? nextBlinkFrameTime * numBlinkSeq : 50;

bool wasBlinking = false;

void updateBlinkSequence() {
    isBlinkEnabled = true;
  switch (currentEyeExpression) {
    case 0:
      selectedBlinkSequence = blinkSequenceRegularEye;
      numBlinkSeq = sizeof(blinkSequenceRegularEye) / sizeof(int);
      break;
    case 2:
      selectedBlinkSequence = blinkSequenceConfusedEye;
      numBlinkSeq = sizeof(blinkSequenceConfusedEye) / sizeof(int);
      break;
    case 3:
        isBlinkEnabled = false;
        break;
    default:
      selectedBlinkSequence = blinkSequenceRegularEye;
      numBlinkSeq = sizeof(blinkSequenceRegularEye) / sizeof(int);
      break;
  }
  blinkDuration = (isAnimatedBlinkEnabled) ? nextBlinkFrameTime * numBlinkSeq : 50;
}

bool isBlinking() {
    if (g.isBoop)  {
        return false;
    }
    if (!isBlinkEnabled) return false;
    
    if ((millis() - lastBlinkMillis) > nextBlinkInterval) {
        lastBlinkMillis = millis();
        nextBlinkInterval = random(6000, 8000);
        return true;
    }
    return (millis() - lastBlinkMillis) < blinkDuration;
}

int lastExpression = currentEyeExpression;

bool lastBoopState = false;

void updateTalking() {
  if (!isTalking) return;

  if (millis() - lastMouthUpdate >= TALK_INTERVAL) {
    lastMouthUpdate = millis();
    mouthFrame = (mouthFrame + 1) % NUM_MOUTH_FRAMES;
    renderFace();
  }
}

unsigned long boopStartMillis = 0;

void updateBoop(bool btn) {
    if (!g.isBoop && btn == LOW) {
        lastExpression = currentEyeExpression;
        currentEyeExpression = 3;
        g.isBoop = true;
        boopStartMillis = millis();
    }
    else if (g.isBoop && btn == HIGH && isTimerFinished(boopStartMillis, 500)) {
        currentEyeExpression = lastExpression;
        g.isBoop = false;
    }

    lastBoopState = btn;
}