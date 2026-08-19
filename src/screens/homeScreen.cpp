#include <screens.h>
#include <inputHandler.h>
#include <timer.h>
#include "homeScreen.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

void screen_home(bool isAudioPass, int co2, int fan, int hum)
{
    if (selIndex >= 0 && (millis() - lastInputTime >= 5000))
    {
        selIndex = -1;
        
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    int barOffset = drawTopBar(16, 44, true, 50);

    u8g2.setFont(u8g2_font_7x13_tr);

    ListRow rows[] = {
        {"CLOCK", 0},
        {"LEDS", 1},
        {"SENSORS", 2},
        {"SETTINGS", 3},
    };

    const int rowCount = 4;
    const int bottomReserved = 10; // reserve room for the AUDIO PASSTHROUGH status text so the list can't clip into it

    int topOffset = listTop + barOffset;
    int visRows = getVisibleRows(topOffset, bottomReserved);

    static int scrollTop = 0;
    int sel = (selIndex >= 0) ? selIndex : 0;
    scrollTop = updateScrollTop(sel, scrollTop, rowCount, visRows);

    for (int i = 0; i < visRows; i++)
    {
        int rowIdx = scrollTop + i;
        if (rowIdx >= rowCount) break;

        ListRow &row = rows[rowIdx];
        bool selected = (selIndex == row.idx);
        drawListRow(i, row.label, selected, topOffset);
    }

    drawScrollArrows(scrollTop, rowCount, visRows, topOffset, 64 - bottomReserved);

    u8g2.drawStr(3, 64, "AUDIO PASSTHROUGH");

    drawMessagePopup(); 

    u8g2.sendBuffer();
}