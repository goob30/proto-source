#pragma once

#include "screens.h" // Screen enum, ROLE_UP / ROLE_DOWN / ROLE_SELECT constants

enum class SettingsScreen : int
{
    ROOT,
    CONTROLS,
    LED,
    MISC,
    SENSORS,
    STYLE,
};

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