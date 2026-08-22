#include "dataHandler.h"
globals g;

void updateData() {
    g.blushLevelPWM = g.blushLevel*40.95; // the blushlevel (percent) converted to digitalWrite scale 0-4095
}