

#pragma once

#include <stdint.h>
#include <U8g2lib.h>
#include <bitmaps.h>
#include <clamp.h>
 

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

extern int currentExpression;
extern int g_brightnessLevel;
extern bool g_isVoiceDetection; 

enum class Screen : int
{
    HOME,
    SETTINGS,
    TELEMETRY,
    CLOCK,
};

extern Screen g_currentScreen;   

void handleInput(int src, int listMax);

void renderFace();

void screenSwitch(Screen screen);

int getMaxScreenIndex(Screen screen);