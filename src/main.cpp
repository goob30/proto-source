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


#define LEFT_NOSE_OFFSET   0
#define LEFT_MOUTH_OFFSET  8
#define LEFT_EYE_OFFSET    40
#define RIGHT_EYE_OFFSET   0
#define RIGHT_MOUTH_OFFSET 16
#define RIGHT_NOSE_OFFSET  48

#define BRIGHTNESS_LEVEL   4 

const int aDet = 33;
const int SAMPLE_WINDOW = 50; 
int baseline = 2048;
int threshold = 50;

void mxDrawBitmap(MD_MAX72XX& mx, const uint8_t* bmp, uint8_t width, uint8_t xOffset) {
  for (uint8_t x = 0; x < width; x++) {
    mx.setColumn(x + xOffset, bmp[x]);
  }
}


void renderFace() {
  mxL.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mxR.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  
  // Clear displays. duurhuhrhhhghughuhuhu
  mxL.clear();
  mxR.clear();

  mxDrawBitmap(mxL, noseL, 8, LEFT_NOSE_OFFSET);
  mxDrawBitmap(mxL, mouthL, 32, LEFT_MOUTH_OFFSET);
  mxDrawBitmap(mxL, eyeL_1, 16, LEFT_EYE_OFFSET);

  mxDrawBitmap(mxR, eyeR_1, 16, RIGHT_EYE_OFFSET);
  mxDrawBitmap(mxR, mouthR, 32, RIGHT_MOUTH_OFFSET);
  mxDrawBitmap(mxR, noseR, 8, RIGHT_NOSE_OFFSET);
  
  
  mxL.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mxR.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}


void setup() {

  pinMode(aDet, INPUT);

  pinMode(BTN_L0, INPUT_PULLUP);
  pinMode(BTN_L1, INPUT_PULLUP);

  // Initialize matrices
  mxL.begin();
  mxR.begin();
  
  // Set brightness (
  mxL.control(MD_MAX72XX::INTENSITY, BRIGHTNESS_LEVEL);
  mxR.control(MD_MAX72XX::INTENSITY, BRIGHTNESS_LEVEL);
  
  renderFace();
  Serial.begin(115200);

  u8g2.begin();
}


void loop() {
  
  screen_switch(0);

  bool curL0 = digitalRead(BTN_L0);
  bool curL1 = digitalRead(BTN_L1);

  lastL0 = curL0;
  lastL1 = curL1;

  // Speech detection
  // unsigned long startMillis = millis();
  // int signalMax = 0;
  // int signalMin = 4095;
  
  // // Fast sampling window
  // while (millis() - startMillis < SAMPLE_WINDOW) {
  //   int sample = analogRead(aDet);
  //   if (sample > signalMax) signalMax = sample;
  //   if (sample < signalMin) signalMin = sample;
  // }
  
  // int peakToPeak = signalMax - signalMin;
  
  // // Auto-adjust baseline during quiet periods
  // if (peakToPeak < 20) {
  //   baseline = (baseline * 9 + ((signalMax + signalMin) / 2)) / 10;
  // }
  
  // // Detect speech (anything above ambient noise)
  // if (peakToPeak > threshold) {
  //   Serial.print("SPEECH! P2P: ");
  //   Serial.print(peakToPeak);
  //   Serial.print(" | Baseline: ");
  //   Serial.println(baseline);
    
  //   // Your animation trigger here
  //   // renderFace(true); 
  // }
  
  delay(10);
}