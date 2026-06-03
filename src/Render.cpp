#include "Render.h"

#include "Assets.h"
#include "Pet.h"

namespace {
constexpr uint8_t RENDER_FPS_SCALE = 3;
constexpr uint8_t renderFramesAtGameFps(uint8_t baseFrames) {
    return baseFrames * RENDER_FPS_SCALE;
}
}

// Convert a menu row index into the shared cursor Y coordinate.
uint8_t menuCursorY(uint8_t selectedAction) {
    return MENU_FIRST_Y + selectedAction * MENU_STEP_Y;
}

// Tinyfont is fixed-width enough that digit count can drive HUD positioning.
uint8_t countDigits(uint16_t value) {
    uint8_t digits = 1;
    while (value >= 10) {
        value /= 10;
        digits++;
    }

    return digits;
}

// Draws the small circular button prompt used by action scenes.
void drawButtonHint(Arduboy2 &arduboy, Tinyfont &tinyfont, uint8_t y, const char *label) {
    arduboy.fillCircle(96, y + 1, 3, BLACK);
    tinyfont.setCursor(102, y);
    tinyfont.print(label);
}

// Default idle-screen life bar position.
void drawLifeBar(Arduboy2 &arduboy) {
    drawLifeBarAt(arduboy, 2);
}

// Life is stored in half-heart units, so three sprites cover all six states.
void drawLifeBarAt(Arduboy2 &arduboy, int16_t x) {
    switch (pet.life)
    {
    case 6:
        Sprites::drawOverwrite(x, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 9, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 18, 1, heartFull, 0);
        break;
    case 5:
        Sprites::drawOverwrite(x, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 9, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 18, 1, heartHalf, 0);
        break;
    case 4:
        Sprites::drawOverwrite(x, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 9, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 18, 1, heartEmpty, 0);
        break;
    case 3:
        Sprites::drawOverwrite(x, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 9, 1, heartHalf, 0);
        Sprites::drawOverwrite(x + 18, 1, heartEmpty, 0);
        break;
    case 2:
        Sprites::drawOverwrite(x, 1, heartFull, 0);
        Sprites::drawOverwrite(x + 9, 1, heartEmpty, 0);
        Sprites::drawOverwrite(x + 18, 1, heartEmpty, 0);
        break;
    case 1:
        // Blink the last half-heart to make critical hunger readable.
        if ((arduboy.frameCount / renderFramesAtGameFps(7)) % 2 == 0) {
            Sprites::drawOverwrite(x, 1, heartHalf, 0);
            Sprites::drawOverwrite(x + 9, 1, heartEmpty, 0);
            Sprites::drawOverwrite(x + 18, 1, heartEmpty, 0);
        }
        break;
    default:
        break;
    }
}

// XP fill is proportional to the dynamic width used by the sliding menu.
void drawXpBar(Arduboy2 &arduboy, Tinyfont &tinyfont, int16_t x, uint8_t width) {
    uint8_t innerWidth = width > 4 ? width - 4 : 0;
    uint8_t barFillWidth = (uint8_t) ((uint16_t) innerWidth * pet.xpTenths / 10);
    arduboy.drawRoundRect(x, 57, width, 6, 1, BLACK);
    arduboy.fillRect(x + 2, 59, barFillWidth, 2, BLACK);

    tinyfont.setCursor(x + 1, 52);
    tinyfont.print("LVL ");
    tinyfont.print(pet.level);
}

// Align the money HUD from the right edge while allowing more digits over time.
void drawMoneyHud(Tinyfont &tinyfont) {
    uint8_t digitCount = countDigits(pet.money);
    tinyfont.setCursor(119 - (digitCount * 5), 2);
    tinyfont.print("$");
    tinyfont.print(pet.money);
}

// Draw the right-side action drawer; x is animated by the idle scene.
void drawMenu(Arduboy2 &arduboy, int16_t x, uint8_t selectedAction) {
    if (x >= 128) {
        return;
    }

    arduboy.drawFastHLine(x + 1, 0, MENU_WIDTH - 2, BLACK);
    arduboy.drawFastHLine(x + 1, 63, MENU_WIDTH - 2, BLACK);
    arduboy.drawFastVLine(x, 1, 62, BLACK);
    arduboy.drawFastVLine(x + MENU_WIDTH - 1, 1, 62, BLACK);

    arduboy.setCursor(x + 10, 4);
    arduboy.print("Feed");
    arduboy.setCursor(x + 10, 16);
    arduboy.print("Play");
    arduboy.setCursor(x + 10, 28);
    arduboy.print("Clean");
    arduboy.setCursor(x + 10, 40);
    arduboy.print("Walk");
    arduboy.setCursor(x + 10, 52);
    arduboy.print("Water");

    arduboy.fillRect(x + 1, 1, 7, 62, WHITE);
    arduboy.drawFastVLine(x + 2, 29, 5, BLACK);
    arduboy.fillCircle(x + 5, menuCursorY(selectedAction) + 3, 1, BLACK);
}
