#pragma once

#include <stdint.h>
#include <U8g2lib.h>
#include <bitmaps.h>
#include "dataHandler.h"
#include "inputHandler.h"
#include "screens/clockScreen.h"
#include "screens/settingsScreen.h"
#include "screens/homeScreen.h"
#include "errorHandler.h"
#include "tools.h"

 

#define FONT_NCEN u8g2_font_ncenB08_tr
#define FONT_MANIAC u8g2_font_maniac_te
#define FONT_BTB u8g2_font_Born2bSportyV2_te
#define FONT_813 u8g2_font_8x13_m_symbols


// constexpr int BTN_INDEX_L = 32;
// constexpr int BTN_MIDDLE_L = 33;
// constexpr int BTN_INDEX_R = 25;
// constexpr int BTN_MIDDLE_R = 14;

// extern const int buttonPins[NUM_BUTTONS];
// extern ButtonRole buttonRole[NUM_BUTTONS];
// extern bool buttonLastState[NUM_BUTTONS];

extern int currentEyeExpression;
// extern int g_brightnessLevel;
extern bool g_isVoiceDetection; 

// enum class Screen : int
// {
//     HOME,
//     SETTINGS,
//     TELEMETRY,
//     CLOCK,
// };

extern Screen g_currentScreen;   

void handleInput(int src, int listMax);

void renderFace();

void screenSwitch(Screen screen);

int getMaxScreenIndex(Screen screen);

extern char g_messagePopupText[128];
void popup(const char *msg, bool isError);

extern const int rectW;
extern const int rowH;
extern const int rowGap;
extern const int visibleRows;
extern const int listTop;
extern const int topBarH;

struct ListRow
{
    const char *label;
    int idx;
};

struct ValueRow
{
    const char *fmt;
    int val;
    int y;
    int idx;
};

// global methods for any screen
int updateScrollTop(int sel, int scrollTop, int rowCount, int visRows);
int getVisibleRows(int topOffset, int bottomReserved);
int drawListRow(int rowSlot, const char *label, bool selected, int topOffset);
void drawScrollArrows(int scrollTop = 0, int rowCount = 0, int visRows = 0, int topOffset = 0, int bottomLimit = 0);
int drawTopBar(int hh, int mm, bool bt, int bat);
void drawScrollingText(const char **labels, int count, int x, int y, int width, int gapPx, float speedPxPerSec);

// popup
extern char g_messagePopupText[128];
extern bool g_messagePopupActive;
extern bool g_messagePopupIsError;
void drawMessagePopup();
void popup(const char *msg, bool isError);

// idk
extern bool g_isTopBarEnabled;
extern const char* clockStyleList[];
extern const uint8_t* clockStyleFont[];
extern const char* expressionList[];

void screenSwitch(Screen screen);