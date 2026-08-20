#pragma once

#include "esp_system.h"
#include "esp32-hal.h"
#include "soc/soc.h"
#include "soc/rtc.h"

extern char g_lastBreadcrumb[32];   // last known-good checkpoint before a crash
// RTC_NOINIT_ATTR uint32_t g_bootCount;

void setBreadcrumb(const char *tag);

const char* resetReasonToStr(esp_reset_reason_t r);

const char* initIfCrash();