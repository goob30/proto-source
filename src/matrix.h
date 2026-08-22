#pragma once

#include <MD_MAX72xx.h>
#include <SPI.h>

#include <Wire.h>

#define CLK_PINR    18
#define DATA_PINR   23
#define CS_PINR     19

#define CLK_PINL    14
#define DATA_PINL   27
#define CS_PINL     13

#define MAX_DEVICES 7

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define BLUSH_PIN_R 26 // chagne this duh

void renderFace();

void updateLeds();

void initLeds();

void mxDrawBitmap(MD_MAX72XX& mx, const uint8_t* bmp, uint8_t width, uint8_t xOffset);

void initMatrices();