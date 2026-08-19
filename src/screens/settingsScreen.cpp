#include <screens.h>
#include <inputHandler.h>
#include <timer.h>
#include "settingsScreen.h"

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;


// ************* all defined in screens.h, js keeping it here in case of sumn
// extern int g_brightnessLevel;

// char g_messagePopupText[128] = "";
// bool g_messagePopupActive = false;
// bool g_messagePopupIsError = false;




// const char* clockStyleList[] = {"STANDARD", "RETRO", "CHUNKY", "SEGMENT"};
// const uint8_t* clockStyleFont[] = {
//     u8g2_font_logisoso28_tn,
//     u8g2_font_VCR_OSD_mn,
//     u8g2_font_freedoomr25_tn,
//     u8g2_font_7Segments_26x42_mn
// };


// bool g_isTopBarEnabled = true;

// const char* expressionList[] = {"DEFAULT", "MOD 1", "MOD 2", "MOD 3", "MOD 4"};
// ********************

void screen_settings(bool isAudioPass)
{
    if (selIndex >= 0 && (millis() - lastInputTime > 5000))
    {
        selIndex = -1;
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    int barOffset = drawTopBar(16, 44, true, 50);

    u8g2.setFont(u8g2_font_7x13_tr);

    ListRow rows[] = {
        {"< RETURN", 0}, // had 2 spaces making it off center, fixed that
        {"CONTROLS", 1},
        {"LED", 2},
        {"MISC", 3},
        {"SENSORS", 4},
        {"STYLE", 5},
    };
    const int rowCount = 6;

    int topOffset = listTop + barOffset;
    int visRows = getVisibleRows(topOffset, 0);

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

    drawScrollArrows(scrollTop, rowCount, visRows, topOffset, 64);

    drawMessagePopup(); 

    u8g2.sendBuffer();
}


void screen_settings_led()
{
    if (selIndex >= 0 && (millis() - lastInputTime > 5000))
    {
        selIndex = -1;
        g_LedSettingsScreenMode = LedSettingsScreenMode::LIST;
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    int barOffset = drawTopBar(16, 44, true, 50);

    u8g2.setFont(u8g2_font_7x13_tr);

    ListRow rows[] = {
        {"< RETURN", 0},
        {"BRIGHTNESS", 1},
        {"EXPRESSION", 2},
        {"VOICE DETECTION", 3},
    };
    const int rowCount = 4; // this is for the clamps in handleinput

    int topOffset = listTop + barOffset;
    int visRows = getVisibleRows(topOffset, 0);

    static int scrollTop = 0;
    int sel = (selIndex >= 0) ? selIndex : 0;
    scrollTop = updateScrollTop(sel, scrollTop, rowCount, visRows);

    // base listt
    for (int i = 0; i < visRows; i++)
    {
        int rowIdx = scrollTop + i;
        if (rowIdx >= rowCount) break;

        ListRow &row = rows[rowIdx];
        bool selected = (selIndex == row.idx);

        String buf;
        const char *label = row.label;

        
        if (row.idx == 1 && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_BRIGHTNESS)
        {
            buf = "-   " + String(g_brightnessLevel) + "   +";
            label = buf.c_str();
        }

        if (row.idx == 3 && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_ISVOICEDETECTION)
        {
            label = ""; // blank out "VOICE DETECTION"; drawn as custom glyph+text block below instead
        }
        

        int textY = drawListRow(i, label, selected, topOffset);

        if (row.idx == 3 && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_ISVOICEDETECTION)
        {
            const int glyphW = 12;
            const char *onStr = "ON";
            const char *offStr = "OFF";
            const int gap = u8g2.getStrWidth("      ");

            u8g2.setFont(u8g2_font_7x13_tr);
            int onW = u8g2.getStrWidth(onStr);
            int offW = u8g2.getStrWidth(offStr);

            int blockW = glyphW + onW + gap + glyphW + offW;
            int cx = (rectW - blockW) / 2;
            int gy = textY;

            u8g2.setDrawColor(selected ? 0 : 1);

            u8g2.setFont(u8g2_font_7x13_t_symbols);
            u8g2.drawGlyph(cx, gy, g_isVoiceDetection ? 0x25CF : 0x25CB);
            cx += glyphW;

            u8g2.setFont(u8g2_font_7x13_tr);
            u8g2.drawStr(cx, gy, onStr);
            cx += onW + gap;

            u8g2.setFont(u8g2_font_7x13_t_symbols);
            u8g2.drawGlyph(cx, gy, !g_isVoiceDetection ? 0x25CF : 0x25CB);
            cx += glyphW;

            u8g2.setFont(u8g2_font_7x13_tr);
            u8g2.drawStr(cx, gy, offStr);

            u8g2.setDrawColor(1); 
        }

        // u8g2.setDrawColor(selected ? 0 : 1);
    }

    drawScrollArrows(scrollTop, rowCount, visRows, topOffset, 64);

    // expression pop up hurrr durrr
    if (g_LedSettingsScreenMode == LedSettingsScreenMode::EXPRESSION_POPUP)
    {
        const int popX = 10, popY = 10, popW = 108, popH = 44;
        const int popRowH = 12;
        const int popVisible = 3;
        u8g2.setDrawColor(0);
        u8g2.drawBox(popX, popY, popW, popH);   // clear background under popup
        u8g2.setDrawColor(1);
        u8g2.drawRFrame(popX, popY, popW, popH, 4);
        int popScrollTop = clamp(g_expressionSelIndex - popVisible / 2, 0, expressionCount - popVisible);
        for (int i = 0; i < popVisible; i++)
        {
            int idx = popScrollTop + i;
            if (idx >= expressionCount) break;
            int rowTop = popY + 2 + i * popRowH;
            int y = rowTop + 9;
            bool sel = (idx == g_expressionSelIndex);
            if (sel)
            {
                u8g2.drawBox(popX + 2, rowTop, popW - 4, popRowH - 1);
                u8g2.setDrawColor(0);
                u8g2.drawStr(popX + 6, y + 1, expressionList[idx]);
                u8g2.setDrawColor(1);
            }
            else
            {
                u8g2.drawStr(popX + 6, y, expressionList[idx]);
            }
        }
    }

    drawMessagePopup(); 

    u8g2.sendBuffer();
}



void screen_settings_style() {
    if (selIndex >= 0 && (millis() - lastInputTime > 5000)) {
        selIndex = -1;
        g_StyleSettingsScreenMode = StyleSettingsScreenMode::LIST;
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_7x13_tr);

    ListRow rows[] = {
        {"< RETURN", 0},
        {"Enable top bar", 0}
    };
    const int rowCount = 2;
    static int scrollTop = 0;
    int sel = (selIndex >= 0) ? selIndex : 0;
    scrollTop = updateScrollTop(sel, scrollTop, rowCount, visibleRows);
    
    for (int i = 0; i < visibleRows; i++) {
        int rowidx = scrollTop + i;
        if (rowidx >= rowCount) break;

        ListRow &row = rows[rowidx];
        bool selected = (selIndex == row.idx);

        
    }

}