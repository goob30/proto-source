#include "esp_system.h"
#include "esp32-hal.h"
#include "soc/soc.h"
#include "soc/rtc.h"
#include "errorHandler.h"

RTC_DATA_ATTR char g_lastBreadcrumb[32] = "";   // last known-good checkpoint before a crash
// RTC_NOINIT_ATTR uint32_t g_bootCount;

void setBreadcrumb(const char *tag) {
    strncpy(g_lastBreadcrumb, tag, sizeof(g_lastBreadcrumb) - 1);
    g_lastBreadcrumb[sizeof(g_lastBreadcrumb) - 1] = '\0';
}

const char* resetReasonToStr(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_PANIC: return "PANIC";
        case ESP_RST_INT_WDT: return "INT WATCHDOG";
        case ESP_RST_TASK_WDT: return "TASK WATCHDOG";
        case ESP_RST_WDT: return "WATCHDOG";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        default: return "UNKNOWN";
    }
}

char crashText[128];

const char* initIfCrash() {
    esp_reset_reason_t crashReason = esp_reset_reason();
    if (crashReason == ESP_RST_PANIC || crashReason == ESP_RST_INT_WDT ||
        crashReason == ESP_RST_TASK_WDT || crashReason == ESP_RST_WDT) {

        
        snprintf(crashText, sizeof(crashText),
                 "%s\nnear: %s",
                 resetReasonToStr(crashReason),
                 g_lastBreadcrumb[0] ? g_lastBreadcrumb : "unknown");

        return crashText;
    }
    return nullptr;
}