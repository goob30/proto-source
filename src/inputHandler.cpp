#include "inputHandler.h"
#include "timer.h" // clamp()

// defined by the LED / eyes subsystem elsewhere in the codebase
extern int g_brightnessLevel;
extern int currentEyeExpression;
void renderFace();

// popup state still lives in screens.cpp (it's part of the rendering layer),
// but handleInput needs to check/clear it
extern bool g_messagePopupActive;

int selIndex = -1;
int prevSelIndex = -1;
unsigned long lastInputTime = 0;

Screen g_currentScreen = Screen::HOME;
SettingsScreen g_currentSettingsScreen = SettingsScreen::ROOT;

LedSettingsScreenMode g_LedSettingsScreenMode = LedSettingsScreenMode::LIST; // sets the active enum index to not be changing any settings
StyleSettingsScreenMode g_StyleSettingsScreenMode = StyleSettingsScreenMode::LIST;
ClockMode g_clockMode = ClockMode::STD;

int g_expressionSelIndex = 0;
bool g_isVoiceDetection = false;

const int clockStyleCount = 3;
const int expressionCount = 5; // not starting at 0, total count


Screen getScreenForSelection(Screen current, int index) {
    switch (current) {
        case Screen::HOME:
            if (index == 3) return Screen::SETTINGS;
            if (index == 0) return Screen::CLOCK;
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
    if (g_messagePopupActive)
    {
        if (src == ROLE_SELECT) g_messagePopupActive = false;
        lastInputTime = millis();
        return;
    }

    bool onLed = (g_currentScreen == Screen::SETTINGS && g_currentSettingsScreen == SettingsScreen::LED);

    if (onLed && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_BRIGHTNESS)
    {
        if (src == ROLE_UP) g_brightnessLevel = clamp(g_brightnessLevel + 1, 0, 15);
        else if (src == ROLE_DOWN) g_brightnessLevel = clamp(g_brightnessLevel - 1, 0, 15);
        else if (src == ROLE_SELECT) g_LedSettingsScreenMode = LedSettingsScreenMode::LIST; // confirm value, drop back to list nav ADD COMMAND HERE
        lastInputTime = millis();
        return;
    }

    if (onLed && g_LedSettingsScreenMode == LedSettingsScreenMode::EXPRESSION_POPUP)
    {
        if (src == ROLE_UP) g_expressionSelIndex = clamp(g_expressionSelIndex + 1, 0, expressionCount - 1);
        else if (src == ROLE_DOWN) g_expressionSelIndex = clamp(g_expressionSelIndex - 1, 0, expressionCount - 1);
        else if (src == ROLE_SELECT) {
            g_LedSettingsScreenMode = LedSettingsScreenMode::LIST;
            currentEyeExpression = g_expressionSelIndex;
            renderFace();
        }
        lastInputTime = millis();
        return;
    }

    if (onLed && g_LedSettingsScreenMode == LedSettingsScreenMode::EDIT_ISVOICEDETECTION)
    {
        if (src == ROLE_UP || src == ROLE_DOWN) g_isVoiceDetection = !g_isVoiceDetection;
        else if (src == ROLE_SELECT) {
            g_LedSettingsScreenMode = LedSettingsScreenMode::LIST;
        }
        lastInputTime = millis();
        return;
    }

    if (g_currentScreen == Screen::CLOCK)
    {
        if (src == ROLE_UP) g_clockMode = static_cast<ClockMode>(clamp(static_cast<int>(g_clockMode) + 1, 0, clockStyleCount - 1));
        else if (src == ROLE_DOWN) g_clockMode = static_cast<ClockMode>(clamp(static_cast<int>(g_clockMode) - 1, 0, clockStyleCount - 1));
        else if (src == ROLE_SELECT) g_currentScreen = Screen::HOME;
        lastInputTime = millis();
        return;
    }

    if (src == ROLE_UP) {
        selIndex = clamp(selIndex + 1, 0, getMaxScreenIndex(g_currentScreen));
    } else if (src == ROLE_DOWN) {
        selIndex = clamp(selIndex - 1, 0, getMaxScreenIndex(g_currentScreen));
    } else if (src == ROLE_SELECT) {
        if (selIndex >= 0) {
            if (g_currentScreen == Screen::SETTINGS) {
                if (g_currentSettingsScreen == SettingsScreen::ROOT && selIndex == 0) {
                    g_currentScreen = Screen::HOME;
                    selIndex = -1;
                } else if (g_currentSettingsScreen != SettingsScreen::ROOT && selIndex == 0) {
                    g_currentSettingsScreen = SettingsScreen::ROOT;
                    selIndex = -1;
                } else if (g_currentSettingsScreen == SettingsScreen::ROOT) {
                    g_currentSettingsScreen = getSettingsScreenForSelection(g_currentSettingsScreen, selIndex);
                    selIndex = -1;
                } else if (g_currentSettingsScreen == SettingsScreen::LED) {
                    if (selIndex == 1) {g_LedSettingsScreenMode = LedSettingsScreenMode::EDIT_BRIGHTNESS;}
                    else if (selIndex == 2) {g_LedSettingsScreenMode = LedSettingsScreenMode::EXPRESSION_POPUP;}
                    else if (selIndex == 3) {g_LedSettingsScreenMode = LedSettingsScreenMode::EDIT_ISVOICEDETECTION;}
                }
            } else {
                Screen t = getScreenForSelection(g_currentScreen, selIndex);
                if (t != g_currentScreen) {
                    g_currentScreen = t;
                    selIndex = -1;
                }
            }
        }
    }
    lastInputTime = millis();
    Serial.println(selIndex);
}