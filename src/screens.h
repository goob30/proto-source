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

constexpr int BTN_INDEX_L = 32;
constexpr int BTN_MIDDLE_L = 33;
constexpr int BTN_INDEX_R = 25;
constexpr int BTN_MIDDLE_R = 14;

enum ButtonRole { ROLE_UP, ROLE_DOWN, ROLE_SELECT, ROLE_MISC, ROLE_COUNT };
constexpr int NUM_BUTTONS = 4;

extern const int buttonPins[NUM_BUTTONS];
extern ButtonRole buttonRole[NUM_BUTTONS];
extern bool buttonLastState[NUM_BUTTONS];

extern int currentEyeExpression;
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

extern char g_messagePopupText[128];
void popup(const char *msg, bool isError);