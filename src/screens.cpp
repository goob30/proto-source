// TODO: add toggleable top bar with clock, selectable telemetry (sensors, bt status)
//       finish all settings options screens & implementation
//       add functionality to draw screens from their respective files/headers instead of hardcoded into here

#include "screens.h"
#include "inputHandler.h"
#include "timer.h"
#include "screens/clockScreen.h"
#include "screens/settingsScreen.h"
#include "screens/homeScreen.h"


extern int g_brightnessLevel;

char g_messagePopupText[128] = "";
bool g_messagePopupActive = false;
bool g_messagePopupIsError = false;

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;


const char* clockStyleList[] = {"STANDARD", "RETRO", "CHUNKY", "SEGMENT"};
const uint8_t* clockStyleFont[] = {
    u8g2_font_logisoso28_tn,
    u8g2_font_VCR_OSD_mn,
    u8g2_font_freedoomr25_tn,
    u8g2_font_7Segments_26x42_mn
};


bool g_isTopBarEnabled = true;

const char* expressionList[] = {"DEFAULT", "MOD 1", "MOD 2", "MOD 3", "MOD 4"};

// forward declarations so functions can reference each other regardless of definition order
void screen_settings(bool isAudioPass);
void screen_settings_led();
void settingsScreenSwitch(SettingsScreen sub);


// ***** SHARED scrollable settings-list styling, usable by any screen *****
const int rectW = 108;
const int rowH = 18;
const int rowGap = 4;
const int visibleRows = 3;
const int listTop = 2;

// height reserved at the top of the screen when the global top bar is enabled
// (one text row + 1px separator line, content below resumes after this)
const int topBarH = 10;


// struct ListRow
// {
//     const char *label;
//     int idx;
// };

// struct ValueRow
// {
//     const char *fmt;
//     int val;
//     int y;
//     int idx;
// };
// defined in screns.h

int updateScrollTop(int sel, int scrollTop, int rowCount, int visRows)
{
    if (sel < scrollTop) scrollTop = sel;
    if (sel > scrollTop + visRows - 1) scrollTop = sel - visRows + 1;
    if (scrollTop > rowCount - visRows) scrollTop = rowCount - visRows;
    if (scrollTop < 0) scrollTop = 0;
    return scrollTop;
}

// works out how many rows can actually fit given whatever's reserved at the top
// (e.g. the global top bar) and bottom (e.g. a status line) of the screen,
// capped at visibleRows so callers never get more slots than they laid out for
int getVisibleRows(int topOffset, int bottomReserved)
{
    int available = 64 - topOffset - bottomReserved;
    int rows = (available + rowGap) / (rowH + rowGap);
    if (rows < 1) rows = 1;
    if (rows > visibleRows) rows = visibleRows;
    return rows;
}

int drawListRow(int rowSlot, const char *label, bool selected, int topOffset)
{
    int y = topOffset + rowSlot * (rowH + rowGap);

    u8g2.setDrawColor(1);
    if (selected)
    {
        u8g2.drawRBox(0, y, rectW, rowH, 4);
        u8g2.setDrawColor(0);
    }
    else
    {
        u8g2.drawRFrame(0, y, rectW, rowH, 4);
    }

    int textW = u8g2.getStrWidth(label);
    int textX = (rectW - textW) / 2;
    int textY = y + rowH / 2 + 4;
    u8g2.drawStr(textX, textY, label);

    u8g2.setDrawColor(1);
    return textY; // handed back so callers can align custom overlays (e.g. glyph rows) to the same baseline
}

void drawScrollArrows(int scrollTop, int rowCount, int visRows, int topOffset, int bottomLimit)
{
    int arrowCx = rectW + 12;

    if (scrollTop > 0)
    {
        u8g2.drawLine(arrowCx - 4, topOffset + 8, arrowCx, topOffset + 2);
        u8g2.drawLine(arrowCx, topOffset + 2, arrowCx + 4, topOffset + 8);
    }
    if (scrollTop + visRows < rowCount)
    {
        u8g2.drawLine(arrowCx - 4, bottomLimit - 10, arrowCx, bottomLimit - 4);
        u8g2.drawLine(arrowCx, bottomLimit - 4, arrowCx + 4, bottomLimit - 10);
    }
}

// draws the global top bar (currently just the clock) when enabled, then restores
// the font that was active before the call so callers can set their own font after. 
// returns the pixel offset content should start at (0 when the bar is disabled), so screens can do int topOffset = listTop + drawTopBar()
int drawTopBar(int hh, int mm, bool bt, int bat)
{
    if (!g_isTopBarEnabled) return 0;

    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tr);
    char clockstr[6];
    snprintf(clockstr, sizeof(clockstr), "%02d:%02d", hh, mm);

    u8g2.drawStr(2, 8, clockstr); // TODO: wire to selectable telemetry per top-level TODO
    u8g2.drawHLine(0, 9, 128); 

    return topBarH;
}

// Horizontally scrolls one or more labels through a fixed-width window, wrapping
// around once every label has passed through (marquee-style). Usable anywhere.
//   labels        - array of strings to cycle through
//   count         - number of entries in labels
//   x, y          - draw position, y is the text baseline (same convention as drawStr)
//   width         - pixel width of the visible scroll window
//   gapPx         - pixel gap inserted between the end of one label and the start of the next
//   speedPxPerSec - scroll speed in pixels per second
// Animation is driven off millis(), so it can just be called every frame with no
// extra state kept by the caller.
void drawScrollingText(const char **labels, int count, int x, int y, int width, int gapPx, float speedPxPerSec)
{
    if (count <= 0) return;

    int totalW = 0;
    for (int i = 0; i < count; i++)
    {
        totalW += u8g2.getStrWidth(labels[i]) + gapPx;
    }
    if (totalW <= 0) return;

    unsigned long now = millis();
    long offset = (long)(((float)now * speedPxPerSec) / 1000.0f) % totalW;

    u8g2.setClipWindow(x, y - u8g2.getMaxCharHeight(), x + width, y + 2);

    int drawX = x - (int)offset;
    int guard = 0;
    while (drawX < x + width && guard < count * 4)
    {
        for (int i = 0; i < count && drawX < x + width; i++)
        {
            int labelW = u8g2.getStrWidth(labels[i]);
            if (drawX + labelW > x)
            {
                u8g2.drawStr(drawX, y, labels[i]);
            }
            drawX += labelW + gapPx;
            guard++;
        }
    }

    u8g2.setMaxClipWindow(); // restore full-screen clipping for subsequent draws
}


void drawMessagePopup()
{
    if (!g_messagePopupActive) return;
    bool isLog = g_messagePopupIsError; 

    if (isLog) {
        // error screen

        const int screenW = 128;
        const int screenH = 64;

        const int btnW = 40;
        const int btnH = 14;
        const int btnX = (screenW - btnW) / 2;
        const int btnY = screenH - btnH - 2;

        u8g2.setFont(u8g2_font_5x7_tr);

        const int lineH = 8;
        const int maxTextW = 124;
        const int maxLines = 6;

        const char* lines[maxLines];
        int lineCount = 0;

        char buf[128];
        strncpy(buf, g_messagePopupText, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';

        char* saveptr;
        char* segment = strtok_r(buf, "\n", &saveptr);

        while (segment && lineCount < maxLines) {
            char* p = segment;
            char lineBuf[32] = "";

            while (*p && lineCount < maxLines) {
                char* wordEnd = strchr(p, ' ');
                char word[32];

                int wlen = wordEnd ? (wordEnd - p) : (int)strlen(p);

                if (wlen >= (int)sizeof(word))
                    wlen = sizeof(word) - 1;

                strncpy(word, p, wlen);
                word[wlen] = '\0';

                char trial[32];

                if (lineBuf[0])
                    snprintf(trial, sizeof(trial), "%s %s", lineBuf, word);
                else
                    snprintf(trial, sizeof(trial), "%s", word);

                if (u8g2.getStrWidth(trial) > maxTextW && lineBuf[0]) {
                    lines[lineCount++] = strdup(lineBuf);
                    lineBuf[0] = '\0';

                    if (lineCount < maxLines)
                        snprintf(lineBuf, sizeof(lineBuf), "%s", word);
                } else {
                    strncpy(lineBuf, trial, sizeof(lineBuf) - 1);
                    lineBuf[sizeof(lineBuf) - 1] = '\0';
                }

                p += wlen;

                if (*p == ' ')
                    p++;
            }

            if (lineBuf[0] && lineCount < maxLines)
                lines[lineCount++] = strdup(lineBuf);

            segment = strtok_r(NULL, "\n", &saveptr);
        }

        u8g2.setDrawColor(0);
        u8g2.drawBox(0, 0, screenW, screenH);

        u8g2.setDrawColor(1);

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(2, 9, "ERROR");

        u8g2.drawHLine(0, 11, screenW);

        u8g2.setFont(u8g2_font_5x7_tr);

        int textY = 20;

        for (int i = 0; i < lineCount; i++) {
            u8g2.drawStr(2, textY, lines[i]);
            textY += lineH;
            free((void*)lines[i]);
        }

        u8g2.drawRBox(btnX, btnY, btnW, btnH, 3);

        u8g2.setDrawColor(0);
        u8g2.setFont(u8g2_font_6x10_tr);

        int okW = u8g2.getStrWidth("OK");
        u8g2.drawStr(btnX + (btnW - okW) / 2, btnY + 10, "OK");

        u8g2.setDrawColor(1);

        return;
    }

    // normal poup

    const int popX = 10;
    const int popY = 10;
    const int popW = 108;
    const int popH = 60;

    const int btnW = 40;
    const int btnH = 14;

    const int btnX = popX + (popW - btnW) / 2;
    const int btnY = popY + popH - btnH - 4;

    u8g2.setFont(u8g2_font_7x13_tr);

    const int lineH = 15;
    const int maxTextW = popW - 8;
    const int maxLines = 2;

    const char* lines[maxLines];
    int lineCount = 0;

    char buf[128];

    strncpy(buf, g_messagePopupText, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char* saveptr;
    char* segment = strtok_r(buf, "\n", &saveptr);

    while (segment && lineCount < maxLines) {
        char* p = segment;
        char lineBuf[32] = "";

        while (*p && lineCount < maxLines) {
            char* wordEnd = strchr(p, ' ');
            char word[32];

            int wlen = wordEnd ? (wordEnd - p) : (int)strlen(p);

            if (wlen >= (int)sizeof(word)) wlen = sizeof(word) - 1;

            strncpy(word, p, wlen);
            word[wlen] = '\0';

            char trial[32];

            if (lineBuf[0]) snprintf(trial, sizeof(trial), "%s %s", lineBuf, word);
            else snprintf(trial, sizeof(trial), "%s", word);

            if (u8g2.getStrWidth(trial) > maxTextW && lineBuf[0]) {
                lines[lineCount++] = strdup(lineBuf);
                lineBuf[0] = '\0';

                if (lineCount < maxLines)
                    snprintf(lineBuf, sizeof(lineBuf), "%s", word);
            } else {
                strncpy(lineBuf, trial, sizeof(lineBuf) - 1);
                lineBuf[sizeof(lineBuf) - 1] = '\0';
            }
            p += wlen;
            if (*p == ' ') p++;
        }

        if (lineBuf[0] && lineCount < maxLines) lines[lineCount++] = strdup(lineBuf);

        segment = strtok_r(NULL, "\n", &saveptr);
    }

    u8g2.setDrawColor(0);
    u8g2.drawBox(popX, popY, popW, popH);

    u8g2.setDrawColor(1);
    u8g2.drawRFrame(popX, popY, popW, popH, 4);

    int textY = (lineCount == 1) ? (popY + 22) : (popY + 17);

    for (int i = 0; i < lineCount; i++) {
        int textW = u8g2.getStrWidth(lines[i]);
        int textX = popX + (popW - textW) / 2;

        u8g2.drawStr(textX, textY, lines[i]);

        textY += lineH;
        free((void*)lines[i]);
    }

    u8g2.drawRBox(btnX, btnY, btnW, btnH, 3);

    u8g2.setDrawColor(0);

    int okW = u8g2.getStrWidth("OK");
    u8g2.drawStr(btnX + (btnW - okW) / 2, btnY + btnH - 3, "OK");

    u8g2.setDrawColor(1);
}

void popup(const char *msg, bool isError)
{
    strncpy(g_messagePopupText, msg, sizeof(g_messagePopupText) - 1);
    g_messagePopupText[sizeof(g_messagePopupText) - 1] = '\0';
    g_messagePopupIsError = isError;
    g_messagePopupActive = true;
}






// void screen_home(bool isAudioPass, int co2, int fan, int hum)
// {
//     if (selIndex >= 0 && (millis() - lastInputTime >= 5000))
//     {
//         selIndex = -1;
        
//     }

//     u8g2.clearBuffer();
//     u8g2.setDrawColor(1);

//     int barOffset = drawTopBar(16, 44, true, 50);

//     u8g2.setFont(u8g2_font_7x13_tr);

//     ListRow rows[] = {
//         {"CLOCK", 0},
//         {"LEDS", 1},
//         {"SENSORS", 2},
//         {"SETTINGS", 3},
//     };

//     const int rowCount = 4;
//     const int bottomReserved = 10; // reserve room for the "AUDIO PASSTHROUGH" status text so the list can't clip into it

//     int topOffset = listTop + barOffset;
//     int visRows = getVisibleRows(topOffset, bottomReserved);

//     static int scrollTop = 0;
//     int sel = (selIndex >= 0) ? selIndex : 0;
//     scrollTop = updateScrollTop(sel, scrollTop, rowCount, visRows);

//     for (int i = 0; i < visRows; i++)
//     {
//         int rowIdx = scrollTop + i;
//         if (rowIdx >= rowCount) break;

//         ListRow &row = rows[rowIdx];
//         bool selected = (selIndex == row.idx);
//         drawListRow(i, row.label, selected, topOffset);
//     }

//     drawScrollArrows(scrollTop, rowCount, visRows, topOffset, 64 - bottomReserved);

//     u8g2.drawStr(3, 64, "AUDIO PASSTHROUGH");

//     drawMessagePopup(); 

//     u8g2.sendBuffer();
// }







// void screen_clock_face() {
//     u8g2.clearBuffer();
//     u8g2.setDrawColor(1);

//     int barOffset = drawTopBar(16, 44, true, 50);

//     u8g2.setFont(clockStyleFont[static_cast<int>(g_clockMode)]);
//     int textW = u8g2.getStrWidth("16:42");
//     int textH = u8g2.getMaxCharHeight();
//     int textX = (128 - textW) / 2;
//     int textY = barOffset + ((64 - barOffset) + textH) / 2;
//     u8g2.drawStr(textX, textY, "16:42");

//     drawMessagePopup(); 

//     u8g2.sendBuffer();
// }




// ***********************          ***********************
// *********************** SETTINGS ***********************
// ***********************          ***********************
// void screen_settings(bool isAudioPass)
// {
//     if (selIndex >= 0 && (millis() - lastInputTime > 5000))
//     {
//         selIndex = -1;
//     }

//     u8g2.clearBuffer();
//     u8g2.setDrawColor(1);

//     int barOffset = drawTopBar(16, 44, true, 50);

//     u8g2.setFont(u8g2_font_7x13_tr);

//     ListRow rows[] = {
//         {"< RETURN  ", 0},
//         {"CONTROLS", 1},
//         {"LED", 2},
//         {"MISC", 3},
//         {"SENSORS", 4},
//         {"STYLE", 5},
//     };
//     const int rowCount = 6;

//     int topOffset = listTop + barOffset;
//     int visRows = getVisibleRows(topOffset, 0);

//     static int scrollTop = 0;
//     int sel = (selIndex >= 0) ? selIndex : 0;
//     scrollTop = updateScrollTop(sel, scrollTop, rowCount, visRows);

//     for (int i = 0; i < visRows; i++)
//     {
//         int rowIdx = scrollTop + i;
//         if (rowIdx >= rowCount) break;

//         ListRow &row = rows[rowIdx];
//         bool selected = (selIndex == row.idx);
//         drawListRow(i, row.label, selected, topOffset);
//     }

//     drawScrollArrows(scrollTop, rowCount, visRows, topOffset, 64);

//     drawMessagePopup(); 

//     u8g2.sendBuffer();
// }


// void screen_settings_led()
// {
//     if (selIndex >= 0 && (millis() - lastInputTime > 5000))
//     {
//         selIndex = -1;
//         g_LedSettingsScreenMode = LedSettingsScreenMode::LIST;
//     }

//     u8g2.clearBuffer();
//     u8g2.setDrawColor(1);

//     int barOffset = drawTopBar(16, 44, true, 50);

//     u8g2.setFont(u8g2_font_7x13_tr);

//     ListRow rows[] = {
//         {"< RETURN", 0},
//         {"BRIGHTNESS", 1},
//         {"EXPRESSION", 2},
//         {"VOICE DETECTION", 3},
//     };
//     const int rowCount = 4; // this is for the clamps in handleinput

//     int topOffset = listTop + barOffset;
//     int visRows = getVisibleRows(topOffset, 0);

//     static int scrollTop = 0;
//     int sel = (selIndex >= 0) ? selIndex : 0;
//     scrollTop = updateScrollTop(sel, scrollTop, rowCount, visRows);

//     // base listt
//     for (int i = 0; i < visRows; i++)
//     {
//         int rowIdx = scrollTop + i;
//         if (rowIdx >= rowCount) break;

//         ListRow &row = rows[rowIdx];
//         bool selected = (selIndex == row.idx);

//         String buf;
//         const char *label = row.label;

        
//         if (row.idx == 1 && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_BRIGHTNESS)
//         {
//             buf = "-   " + String(g_brightnessLevel) + "   +";
//             label = buf.c_str();
//         }

//         if (row.idx == 3 && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_ISVOICEDETECTION)
//         {
//             label = ""; // blank out "VOICE DETECTION"; drawn as custom glyph+text block below instead
//         }
        

//         int textY = drawListRow(i, label, selected, topOffset);

//         if (row.idx == 3 && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_ISVOICEDETECTION)
//         {
//             const int glyphW = 12;
//             const char *onStr = "ON";
//             const char *offStr = "OFF";
//             const int gap = u8g2.getStrWidth("      ");

//             u8g2.setFont(u8g2_font_7x13_tr);
//             int onW = u8g2.getStrWidth(onStr);
//             int offW = u8g2.getStrWidth(offStr);

//             int blockW = glyphW + onW + gap + glyphW + offW;
//             int cx = (rectW - blockW) / 2;
//             int gy = textY;

//             u8g2.setDrawColor(selected ? 0 : 1);

//             u8g2.setFont(u8g2_font_7x13_t_symbols);
//             u8g2.drawGlyph(cx, gy, g_isVoiceDetection ? 0x25CF : 0x25CB);
//             cx += glyphW;

//             u8g2.setFont(u8g2_font_7x13_tr);
//             u8g2.drawStr(cx, gy, onStr);
//             cx += onW + gap;

//             u8g2.setFont(u8g2_font_7x13_t_symbols);
//             u8g2.drawGlyph(cx, gy, !g_isVoiceDetection ? 0x25CF : 0x25CB);
//             cx += glyphW;

//             u8g2.setFont(u8g2_font_7x13_tr);
//             u8g2.drawStr(cx, gy, offStr);

//             u8g2.setDrawColor(1); 
//         }

//         u8g2.setDrawColor(selected ? 0 : 1);
//     } // thank you claude 

//     drawScrollArrows(scrollTop, rowCount, visRows, topOffset, 64);

//     // expression pop up hurrr durrr
//     if (g_LedSettingsScreenMode == LedSettingsScreenMode::EXPRESSION_POPUP)
//     {
//         const int popX = 10, popY = 10, popW = 108, popH = 44;
//         const int popRowH = 12;
//         const int popVisible = 3;
//         u8g2.setDrawColor(0);
//         u8g2.drawBox(popX, popY, popW, popH);   // clear background under popup
//         u8g2.setDrawColor(1);
//         u8g2.drawRFrame(popX, popY, popW, popH, 4);
//         int popScrollTop = clamp(g_expressionSelIndex - popVisible / 2, 0, expressionCount - popVisible);
//         for (int i = 0; i < popVisible; i++)
//         {
//             int idx = popScrollTop + i;
//             if (idx >= expressionCount) break;
//             int rowTop = popY + 2 + i * popRowH;
//             int y = rowTop + 9;
//             bool sel = (idx == g_expressionSelIndex);
//             if (sel)
//             {
//                 u8g2.drawBox(popX + 2, rowTop, popW - 4, popRowH - 1);
//                 u8g2.setDrawColor(0);
//                 u8g2.drawStr(popX + 6, y + 1, expressionList[idx]);
//                 u8g2.setDrawColor(1);
//             }
//             else
//             {
//                 u8g2.drawStr(popX + 6, y, expressionList[idx]);
//             }
//         }
//     }

//     drawMessagePopup(); 

//     u8g2.sendBuffer();
// }



// void screen_settings_style() {
//     if (selIndex >= 0 && (millis() - lastInputTime > 5000)) {
//         selIndex = -1;
//         g_StyleSettingsScreenMode = StyleSettingsScreenMode::LIST;
//     }

//     u8g2.clearBuffer();
//     u8g2.setDrawColor(1);
//     u8g2.setFont(u8g2_font_7x13_tr);

//     ListRow rows[] = {
//         {"< RETURN", 0},
//         {"Enable top bar", 0}
//     };
//     const int rowCount = 2;
//     static int scrollTop = 0;
//     int sel = (selIndex >= 0) ? selIndex : 0;
//     scrollTop = updateScrollTop(sel, scrollTop, rowCount, visibleRows);
    
//     for (int i = 0; i < visibleRows; i++) {
//         int rowidx = scrollTop + i;
//         if (rowidx >= rowCount) break;

//         ListRow &row = rows[rowidx];
//         bool selected = (selIndex == row.idx);

        
//     }

// }
// ***********************              ***********************
// *********************** END SETTINGS ***********************
// ***********************              ***********************



















void settingsScreenSwitch(SettingsScreen sub) {
    switch (sub) {
        case SettingsScreen::ROOT: screen_settings(g_isVoiceDetection); break;
        case SettingsScreen::CONTROLS: break;
        case SettingsScreen::LED: screen_settings_led(); break;
        case SettingsScreen::MISC: break;
        case SettingsScreen::SENSORS: break;
        case SettingsScreen::STYLE: break;
    }
}


void screenSwitch(Screen screen)
{
    g_currentScreen = screen;
    switch (screen)
    {
    case Screen::HOME:
        screen_home(g_isVoiceDetection, globalCo2, globalFan, globalHum);
        break;
    case Screen::SETTINGS:
        settingsScreenSwitch(g_currentSettingsScreen);
        break;
    case Screen::CLOCK:
        screen_clock_face();
    }
}