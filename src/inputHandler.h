#pragma once

#include "screenTypes.h" // Screen enum, ROLE_UP / ROLE_DOWN / ROLE_SELECT constants
#include "errorHandler.h"
#include "dataHandler.h"
#include "tools.h"

// LIST is when you are only scrolling, not modifying a value
enum class LedSettingsScreenMode : int {LIST, EDIT_BRIGHTNESS, EXPRESSION_POPUP, EDIT_ISVOICEDETECTION};
enum class StyleSettingsScreenMode : int {LIST, EDIT_ISTOPBARENABLED};
enum class ClockMode : int {STD, BMP, SEG};

// nav state
extern int selIndex;
extern int prevSelIndex;
extern unsigned long lastInputTime;

extern Screen g_currentScreen;
extern SettingsScreen g_currentSettingsScreen;
extern LedSettingsScreenMode g_LedSettingsScreenMode;
extern StyleSettingsScreenMode g_StyleSettingsScreenMode;
extern ClockMode g_clockMode;

constexpr int BTN_INDEX_L = 32;
constexpr int BTN_MIDDLE_L = 33;
constexpr int BTN_INDEX_R = 25;
constexpr int BTN_MIDDLE_R = 14;
constexpr int BTN_BOOP = 4;

extern bool lastBoopState;

extern const int buttonPins[NUM_BUTTONS];
extern ButtonRole buttonRole[NUM_BUTTONS];
extern bool buttonLastState[NUM_BUTTONS];

void initButtons();
void pollInputs();

extern int g_expressionSelIndex;
extern bool g_isVoiceDetection;

// bounds used for clamping selection / scrolling popups, kept here bc they define the valid range of the state above
extern const int clockStyleCount;
extern const int expressionCount;

// the thing
void handleInput(int src, int listMax);

Screen getScreenForSelection(Screen current, int index);
SettingsScreen getSettingsScreenForSelection(SettingsScreen current, int index);
int getMaxScreenIndex(Screen screen);