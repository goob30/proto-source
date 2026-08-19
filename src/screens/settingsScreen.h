#pragma once

#include <screens.h>
#include <inputHandler.h>
#include <timer.h>

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
void screen_settings(bool isAudioPass);

void screen_settings_led();

void screen_settings_style();
