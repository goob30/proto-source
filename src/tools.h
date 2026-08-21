inline bool isTimerFinished(unsigned long startMillis, unsigned long durationMillis);

template <typename T>
T clamp(T val, T minVal, T maxVal) {
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
}




template <typename L>
// happens every ms
L lerp(L start, L end, unsigned long startMs, unsigned long durMs);