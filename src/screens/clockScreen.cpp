#include <screens.h>
#include <inputHandler.h>
#include <timer.h>
#include "clockScreen.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

bool isTopBarDuringClockEnabled = false;

void screen_clock_face() {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    int barOffset = isTopBarDuringClockEnabled ? drawTopBar(16, 44, true, 50) : 0;

    u8g2.setFont(clockStyleFont[static_cast<int>(g_clockMode)]);
    int textW = u8g2.getStrWidth("16:42");
    int textH = u8g2.getMaxCharHeight();
    int textX = (128 - textW) / 2;
    int textY = barOffset + ((64 - barOffset) + textH) / 2;
    u8g2.drawStr(textX, textY, "16:42");

    drawMessagePopup(); 

    u8g2.sendBuffer();
}
