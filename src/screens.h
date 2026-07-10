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


Screen g_currentScreen = Screen::HOME;


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
        {"MISC", 2},
        {"SENSORS", 3},
        {"STYLE", 4},
    };
    const int rowCount = 5;

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


Screen getScreenForSelection(Screen current, int index) {
    switch(current) {
        case Screen::HOME:
            if (index == 3) return Screen::SETTINGS;
            return current;
        case Screen::SETTINGS:
            return current;
        default:
            return current;
    }
}


int getMaxScreenIndex(Screen screen)
{
    switch (screen)
    {
    case Screen::HOME: return 3;
    case Screen::SETTINGS: return 4;
    case Screen::TELEMETRY: return 2;
    default: return 0;
    }
}


void handleInput(int src, int listMax)
{
    switch (src)
    {
    case BTN_L0:
        selIndex = clamp(selIndex + 1, 0, getMaxScreenIndex(g_currentScreen));
        break;
    case BTN_L1:
        selIndex = clamp(selIndex - 1, 0, getMaxScreenIndex(g_currentScreen));
        break;
    case BTN_L2:
        Serial.println("l2");
        if (selIndex >= 0) {
            Screen t = getScreenForSelection(g_currentScreen, selIndex);
            if (t != g_currentScreen) {
                g_currentScreen = t;
                selIndex = -1;
            }
        }
        break;
        
    default:
        break;
    }
    lastInputTime = millis();
    Serial.println(selIndex);
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
        screen_settings(isAudioPass);
        break;
    }
}