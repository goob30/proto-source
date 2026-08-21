#pragma once

#include <stdint.h>

typedef struct {
    int brightnessLevel = 0;

    int blushLevel = 0; // percent
    int blushLevelDWrite = blushLevel*40.95; // the blushlevel (percent) converted to digitalWrite scale 0-4095

    int co2 = 30;
    int fan = 100;
    int hum = 30;
    bool isBoop = false;
} globals;

extern globals g;