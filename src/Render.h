#pragma once

#include <Arduboy2.h>
#include <Tinyfont.h>

constexpr uint8_t MENU_ACTION_COUNT = 5;
constexpr uint8_t MENU_FIRST_Y = 4;
constexpr uint8_t MENU_STEP_Y = 12;
constexpr uint8_t MENU_WIDTH = 48;
constexpr uint8_t MENU_OPEN_X = 80;
constexpr uint8_t MENU_CLOSED_X = 125;

uint8_t menuCursorY(uint8_t selectedAction);
void drawButtonHint(Arduboy2Base &arduboy, Tinyfont &tinyfont, uint8_t y, const char *label);
void drawLifeBar(Arduboy2Base &arduboy);
void drawLifeBarAt(Arduboy2Base &arduboy, int16_t x);
void drawXpBar(Arduboy2Base &arduboy, Tinyfont &tinyfont, int16_t x, uint8_t width);
void drawMoneyHud(Tinyfont &tinyfont);
void drawMenu(Arduboy2Base &arduboy, int16_t x, uint8_t selectedAction);
