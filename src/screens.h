#pragma once

#include <stdint.h>
#include <U8g2lib.h>
#include <bitmaps.h>
#include <clamp.h>

// --- Screen / display hardware ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

#define FONT_NCEN   u8g2_font_ncenB08_tr
#define FONT_MANIAC u8g2_font_maniac_te
#define FONT_BTB    u8g2_font_Born2bSportyV2_te
#define FONT_813    u8g2_font_8x13_m_symbols

// --- Screen navigation buttons ---
#define BTN_L0 32
#define BTN_L1 33

bool lastL0 = HIGH;
bool lastL1 = HIGH;

int selIndex = -1;
int prevSelIndex = -1;
unsigned long lastInputTime = 0;

enum class Screen : uint8_t {
  HOME,
  SETTINGS,
  TELEMETRY,
};

// Forward declaration (default args live here only)
void screen_0(boolean isAudioPass = true, uint co2 = 30, uint fan = 100, uint hum = 30);

void handleInput(int8_t src, int listMax) {
  prevSelIndex = selIndex;
  switch (src) {
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

void screen_switch(uint screen) {
  switch (screen) {
    case 0:
      screen_0();
      break;
  }
}

void screen_0(boolean isAudioPass, uint co2, uint fan, uint hum) {

  if (selIndex >= 0 && (millis() - lastInputTime > 5000)) {
    selIndex = -1;
  }

  u8g2.clearBuffer();
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_7x13_tr);

  char buf[32];

  struct Row { const char* fmt; uint val; int y; int idx; };

  Row rows[] = {
    {"CO2 %u%%", co2, 11, 0},
    {"FAN %u%%", fan, 21, 1},
    {"HUMIDITY %u%%", hum, 31, 2},
    {"SETTINGS", 0, 41, 3},
  };

  for (auto& row : rows) {
    snprintf(buf, sizeof(buf), row.fmt, row.val);
    if (selIndex == row.idx) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, row.y - 11, 128, 13); // x, y, w, h
      u8g2.setDrawColor(0);
      u8g2.drawStr(0, row.y, buf);
      u8g2.setDrawColor(1);
    } else {
      u8g2.setDrawColor(1);
      u8g2.drawStr(0, row.y, buf);
    }
  }

  u8g2.drawLine(0, 53, 127, 53);
  snprintf(buf, sizeof(buf), "AUDIO PASSTHROUGH");
  u8g2.drawStr(3, 64, buf);

  u8g2.sendBuffer();
}