#pragma once

#include <stdint.h>

typedef struct {
    int brightnessLevel = 0;

    float blushLevel = 0; // percent
    int blushLevelPWM = 0;

    int co2 = 30;
    int fan = 100;
    int hum = 30;
    bool isBoop = false;
} globals;

extern globals g;

void updateData();