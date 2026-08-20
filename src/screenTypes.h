#pragma once

enum class Screen : int
{
    HOME,
    SETTINGS,
    TELEMETRY,
    CLOCK,
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

enum ButtonRole {ROLE_UP, ROLE_DOWN, ROLE_SELECT, ROLE_MISC, ROLE_COUNT};
constexpr int NUM_BUTTONS = 4;