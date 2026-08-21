#pragma once
#include "screens.h"

inline bool isTimerFinished(unsigned long startMillis, unsigned long durationMillis) {
    if (millis() - startMillis >= durationMillis) {
        return true;
    } else { return false; }
}

template <typename T>
T clamp(T val, T minVal, T maxVal) {
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
}




template <typename L>
// happens every ms
L lerp(L start, L end, unsigned long startMs, unsigned long durMs) {
    unsigned long elapsed = millis() - startMs;
    float didyTime = clamp((float)elapsed / (float)durMs, 0.0f, 1.0f);
    return start + (end - start) * didyTime;
}