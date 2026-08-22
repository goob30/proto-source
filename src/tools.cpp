#include "screens.h"

bool isTimerFinished(unsigned long startMillis, unsigned long durationMillis) {
    if (millis() - startMillis >= durationMillis) {
        return true;
    } else { return false; }
}

    


