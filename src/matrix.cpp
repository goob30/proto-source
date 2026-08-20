#include "matrix.h"
#include "bitmaps.h"
#include "dataHandler.h"


globals g;

MD_MAX72XX mxR = MD_MAX72XX(HARDWARE_TYPE, DATA_PINR, CLK_PINR, CS_PINR, MAX_DEVICES);
MD_MAX72XX mxL = MD_MAX72XX(HARDWARE_TYPE, DATA_PINL, CLK_PINL, CS_PINL, MAX_DEVICES);


void mxDrawBitmap(MD_MAX72XX& mx, const uint8_t* bmp, uint8_t width, uint8_t xOffset) {
  for (uint8_t x = 0; x < width; x++) {
    mx.setColumn(x + xOffset, bmp[x]);
  }
}

void initMatrices() {
    mxL.begin();
    mxR.begin();
  
    mxL.control(MD_MAX72XX::INTENSITY, g.brightnessLevel);
    mxR.control(MD_MAX72XX::INTENSITY, g.brightnessLevel);
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

  mxL.control(MD_MAX72XX::INTENSITY, g.brightnessLevel);
  mxR.control(MD_MAX72XX::INTENSITY, g.brightnessLevel);
}