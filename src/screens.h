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

#define BTN_L0 32
#define BTN_L1 33

extern int globalCo2;
extern int globalFan;
extern int globalHum;

bool lastL0 = HIGH;
bool lastL1 = HIGH;

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


int getMaxScreenIndex(Screen screen)
{
    switch (screen)
    {
    case Screen::HOME: return 3;
    case Screen::SETTINGS: return 5;
    case Screen::TELEMETRY: return 2;
    default: return 0;
    }
}


void handleInput(int src, int listMax)
{
    switch (src)
    {
    case BTN_L0:
        selIndex = clamp(selIndex + 1, 0, listMax);
        break;
    case BTN_L1:
        selIndex = clamp(selIndex - 1, 0, listMax);
        break;
    default:
        break;
    }
    lastInputTime = millis();
    Serial.println(selIndex);
}


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

    u8g2.drawLine(0, 53, 127, 53);
    snprintf(buf, sizeof(buf), "AUDIO PASSTHROUGH");
    u8g2.drawStr(3, 64, buf);

    u8g2.sendBuffer();
}


void screen_switch(Screen screen)
{
    g_currentScreen = screen;
    switch (screen)
    {
    case Screen::HOME:
        screen_home(isAudioPass, globalCo2, globalFan, globalHum);
        break;
    }
}