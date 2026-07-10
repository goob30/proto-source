#pragma once

#include <stdint.h>
#include <U8g2lib.h>
#include <bitmaps.h>
#include <clamp.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

#define FONT_NCEN u8g2_font_ncenB08_tr
#define FONT_MANIAC u8g2_font_maniac_te
#define FONT_BTB u8g2_font_Born2bSportyV2_te
#define FONT_813 u8g2_font_8x13_m_symbols


extern int globalCo2;
extern int globalFan;
extern int globalHum;

constexpr int BTN_L0 = 32;
constexpr int BTN_L1 = 33;
constexpr int BTN_L2 = 25;



bool isAudioPass = false;

int selIndex = -1;
int prevSelIndex = -1;
unsigned long lastInputTime = 0;


enum class Screen : int
{
    HOME,
    SETTINGS,
    TELEMETRY,
};

enum class SettingsScreen : int
{
    ROOT,
    CONTROLS,
    LED,
    MISC,
    SENSORS,
    STYLE,
};

enum class LedMode : int { LIST, EDIT_BRIGHTNESS, EXPRESSION_POPUP };
LedMode g_ledMode = LedMode::LIST;

int g_brightness = 4;
int g_expressionSelIndex = 0;

const char* expressionList[] = { "NEUTRAL", "HAPPY", "ANGRY", "SLEEPY", "SURPRISED" };
const int expressionCount = 5;

Screen g_currentScreen = Screen::HOME;
SettingsScreen g_currentSettingsScreen = SettingsScreen::ROOT;

// forward declarations so functions can reference each other regardless of definition order
void screen_settings(boolean isAudioPass);
void screen_settings_led();
Screen getScreenForSelection(Screen current, int index);
SettingsScreen getSettingsScreenForSelection(SettingsScreen current, int index);
int getMaxScreenIndex(Screen screen);
void settingsScreenSwitch(SettingsScreen sub);


void screen_home(boolean isAudioPass, int co2, int fan, int hum)
{

    if (selIndex >= 0 && (millis() - lastInputTime > 5000))
    {
        selIndex = -1;
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    u8g2.setFont(u8g2_font_7x13_tr);

    char buf[32];

    struct Row
    {
        const char *fmt;
        int val;
        int y;
        int idx;
    };

    Row rows[] = {
        {"CO2 %u%%", co2, 11, 0},
        {"FAN %u%%", fan, 23, 1},
        {"HUMIDITY %u%%", hum, 35, 2},
        {"SETTINGS", 0, 47, 3},
    };

    for (auto &row : rows)
    {
        snprintf(buf, sizeof(buf), row.fmt, row.val);
        if (selIndex == row.idx)
        {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0, row.y - 11, 128, 13); // x, y, w, h
            u8g2.setDrawColor(0);
            u8g2.drawStr(0, row.y, buf);
            u8g2.setDrawColor(1);
        }
        else
        {
            u8g2.setDrawColor(1);
            u8g2.drawStr(0, row.y, buf);
        }
    }

    snprintf(buf, sizeof(buf), "AUDIO PASSTHROUGH");
    u8g2.drawStr(3, 64, buf);

    u8g2.sendBuffer();
}


void screen_settings(boolean isAudioPass)
{
    if (selIndex >= 0 && (millis() - lastInputTime > 5000))
    {
        selIndex = -1;
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_7x13_tr);

    struct Row
    {
        const char *label;
        int idx;
    };

    Row rows[] = {
        {"< RETURN  ", 0},
        {"CONTROLS", 1},
        {"LED", 2},
        {"MISC", 3},
        {"SENSORS", 4},
        {"STYLE", 5},
    };
    const int rowCount = 6;

    const int rectW = 108;
    const int rowH = 18;
    const int rowGap = 4;
    const int visibleRows = 3;
    const int listTop = 2;

    static int scrollTop = 0;
    int sel = (selIndex >= 0) ? selIndex : 0;
    if (sel < scrollTop) scrollTop = sel;
    if (sel > scrollTop + visibleRows - 1) scrollTop = sel - visibleRows + 1;
    if (scrollTop > rowCount - visibleRows) scrollTop = rowCount - visibleRows;
    if (scrollTop < 0) scrollTop = 0;

    for (int i = 0; i < visibleRows; i++)
    {
        int rowIdx = scrollTop + i;
        if (rowIdx >= rowCount) break;

        Row &row = rows[rowIdx];
        int y = listTop + i * (rowH + rowGap);
        bool selected = (selIndex == row.idx);

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

        int textW = u8g2.getStrWidth(row.label);
        int textX = (rectW - textW) / 2;
        int textY = y + rowH / 2 + 4;
        u8g2.drawStr(textX, textY, row.label);

        u8g2.setDrawColor(1);
    }

    int arrowCx = rectW + 12;

    if (scrollTop > 0)
    {
        u8g2.drawLine(arrowCx - 4, 10, arrowCx, 4);
        u8g2.drawLine(arrowCx, 4, arrowCx + 4, 10);
    }
    if (scrollTop + visibleRows < rowCount)
    {
        u8g2.drawLine(arrowCx - 4, 54, arrowCx, 60);
        u8g2.drawLine(arrowCx, 60, arrowCx + 4, 54);
    }

    u8g2.sendBuffer();
}


void screen_settings_led()
{
    if (selIndex >= 0 && (millis() - lastInputTime > 5000))
    {
        selIndex = -1;
    }

    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_7x13_tr);

    struct Row { const char *label; int idx; };
    Row rows[] = {
        {"< RETURN", 0},
        {"BRIGHTNESS", 1},
        {"EXPRESSION", 2},
        {"VOICE DETECTION", 3},
    };
    const int rowCount = 4;   // was 6 — must match the array above

    const int rectW = 108;
    const int rowH = 18;
    const int rowGap = 4;
    const int visibleRows = 3;
    const int listTop = 2;

    static int scrollTop = 0;
    int sel = (selIndex >= 0) ? selIndex : 0;
    if (sel < scrollTop) scrollTop = sel;
    if (sel > scrollTop + visibleRows - 1) scrollTop = sel - visibleRows + 1;
    if (scrollTop > rowCount - visibleRows) scrollTop = rowCount - visibleRows;
    if (scrollTop < 0) scrollTop = 0;

    // --- base list (always drawn, even under the popup) ---
    for (int i = 0; i < visibleRows; i++)
    {
        int rowIdx = scrollTop + i;
        if (rowIdx >= rowCount) break;

        Row &row = rows[rowIdx];
        int y = listTop + i * (rowH + rowGap);
        bool selected = (selIndex == row.idx);

        char buf[24];
        const char *label = row.label;

        // BRIGHTNESS row swaps to "- 4 +" while editing
        if (row.idx == 1 && g_ledMode == LedMode::EDIT_BRIGHTNESS)
        {
            snprintf(buf, sizeof(buf), "-   %d   +", g_brightness);
            label = buf;
        }

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
    }

    int arrowCx = rectW + 12;
    if (scrollTop > 0)
    {
        u8g2.drawLine(arrowCx - 4, 10, arrowCx, 4);
        u8g2.drawLine(arrowCx, 4, arrowCx + 4, 10);
    }
    if (scrollTop + visibleRows < rowCount)
    {
        u8g2.drawLine(arrowCx - 4, 54, arrowCx, 60);
        u8g2.drawLine(arrowCx, 60, arrowCx + 4, 54);
    }

    // --- EXPRESSION popup overlay ---
    if (g_ledMode == LedMode::EXPRESSION_POPUP)
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

    u8g2.sendBuffer();
}


Screen getScreenForSelection(Screen current, int index) {
    switch(current) {
        case Screen::HOME:
            if (index == 3) return Screen::SETTINGS;
            return current;
        case Screen::SETTINGS:
            if (index == 0) return Screen::HOME;
            return current;
        default:
            return current;
    }
}


SettingsScreen getSettingsScreenForSelection(SettingsScreen current, int index) {
    switch (current) {
        case SettingsScreen::ROOT:
            if (index == 0) return SettingsScreen::ROOT; // handled specially below (exits to HOME)
            if (index == 1) return SettingsScreen::CONTROLS;
            if (index == 2) return SettingsScreen::LED;
            if (index == 3) return SettingsScreen::MISC;
            if (index == 4) return SettingsScreen::SENSORS;
            if (index == 5) return SettingsScreen::STYLE;
            return current;
        default:
            return current; // sub-pages handle their own idx==0 "< RETURN" specially
    }
}


int getMaxScreenIndex(Screen screen)
{
    switch (screen)
    {
    case Screen::HOME: return 3;
    case Screen::SETTINGS:
        switch (g_currentSettingsScreen)
        {
        case SettingsScreen::ROOT: return 5;
        case SettingsScreen::LED: return 3;
        default: return 0;
        }
    case Screen::TELEMETRY: return 2;
    default: return 0;
    }
}


void handleInput(int src, int listMax)
{
    bool onLed = (g_currentScreen == Screen::SETTINGS && g_currentSettingsScreen == SettingsScreen::LED);

    if (onLed && g_ledMode == LedMode::EDIT_BRIGHTNESS)
    {
        if (src == BTN_L0) g_brightness = clamp(g_brightness + 1, 0, 15);
        else if (src == BTN_L1) g_brightness = clamp(g_brightness - 1, 0, 15);
        else if (src == BTN_L2) g_ledMode = LedMode::LIST;   // confirm value, drop back to list nav
        lastInputTime = millis();
        return;
    }

    if (onLed && g_ledMode == LedMode::EXPRESSION_POPUP)
    {
        if (src == BTN_L0) g_expressionSelIndex = clamp(g_expressionSelIndex + 1, 0, expressionCount - 1);
        else if (src == BTN_L1) g_expressionSelIndex = clamp(g_expressionSelIndex - 1, 0, expressionCount - 1);
        else if (src == BTN_L2) g_ledMode = LedMode::LIST;   // confirm selection, close popup
        lastInputTime = millis();
        return;
    }

    switch (src)
    {
    case BTN_L0:
        selIndex = clamp(selIndex + 1, 0, getMaxScreenIndex(g_currentScreen));
        break;
    case BTN_L1:
        selIndex = clamp(selIndex - 1, 0, getMaxScreenIndex(g_currentScreen));
        break;
    case BTN_L2:
    if (selIndex >= 0) {
        if (g_currentScreen == Screen::SETTINGS) {
            if (g_currentSettingsScreen == SettingsScreen::ROOT && selIndex == 0) {
                g_currentScreen = Screen::HOME;
                selIndex = -1;
            } else if (g_currentSettingsScreen != SettingsScreen::ROOT && selIndex == 0) {
                g_currentSettingsScreen = SettingsScreen::ROOT; // "< RETURN" on any sub-page
                selIndex = -1;
            } else if (g_currentSettingsScreen == SettingsScreen::ROOT) {
                g_currentSettingsScreen = getSettingsScreenForSelection(g_currentSettingsScreen, selIndex);
                selIndex = -1;
            } else if (g_currentSettingsScreen == SettingsScreen::LED) {
                if (selIndex == 1) { g_ledMode = LedMode::EDIT_BRIGHTNESS; }
                else if (selIndex == 2) { g_ledMode = LedMode::EXPRESSION_POPUP; }
            }
        } else {
            Screen t = getScreenForSelection(g_currentScreen, selIndex);
            if (t != g_currentScreen) {
                g_currentScreen = t;
                selIndex = -1;
            }
        }
    }
    break;
        
    default:
        break;
    }
    lastInputTime = millis();
    Serial.println(selIndex);
}


void settingsScreenSwitch(SettingsScreen sub) {
    switch (sub) {
        case SettingsScreen::ROOT: screen_settings(isAudioPass); break;
        case SettingsScreen::CONTROLS: break;
        case SettingsScreen::LED: screen_settings_led(); break;
        case SettingsScreen::MISC: break;
        case SettingsScreen::SENSORS: break;
        case SettingsScreen::STYLE: break;
    }
}


void screen_switch(Screen screen)
{
    g_currentScreen = screen;
    switch (screen)
    {
    case Screen::HOME:
        screen_home(isAudioPass, globalCo2, globalFan, globalHum);
        break;
    case Screen::SETTINGS:
        settingsScreenSwitch(g_currentSettingsScreen);
        break;
    }
}