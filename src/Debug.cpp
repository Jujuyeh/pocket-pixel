#include "Debug.h"

#if POCKET_PIXEL_DEBUG

#include "Pet.h"
#include "Save.h"

namespace {
constexpr uint8_t ITEM_COUNT = 11;
uint8_t selectedItem = 0;
bool visible = false;

enum DebugItem : uint8_t {
    ITEM_LIFE,
    ITEM_MONEY,
    ITEM_LEVEL,
    ITEM_XP,
    ITEM_DIRTY,
    ITEM_BORED,
    ITEM_ANXIOUS,
    ITEM_SCRATCHING,
    ITEM_SLEEPING,
    ITEM_CLEAR_FLAGS,
    ITEM_RESET_SAVE,
};

// Keep every debug mutation persistent so hardware testing survives reboot.
void savePet() {
    saveGame();
}

// XP is displayed as tenths; debug edits must stay inside the renderable range.
void clampXp() {
    if (pet.xpTenths > 9) {
        pet.xpTenths = 9;
    }
}

// Route status changes through Pet helpers so derived states keep their rules.
void setFlag(PetStatus status, bool enabled) {
    if (enabled) {
        petSet(status);
    } else {
        petUnset(status);
    }
    savePet();
}

// Fast recovery command when random states make a test scenario noisy.
void clearStatuses() {
    pet.statusFlags = 0;
    savePet();
}

// Clamp helper for byte-sized counters edited from the menu.
void adjustUint8(uint8_t &value, int8_t delta, uint8_t minValue, uint8_t maxValue) {
    int16_t adjusted = value + delta;
    if (adjusted < minValue) {
        value = minValue;
    } else if (adjusted > maxValue) {
        value = maxValue;
    } else {
        value = adjusted;
    }
}

// Clamp helper for money, which needs more than one byte.
void adjustUint16(uint16_t &value, int16_t delta, uint16_t minValue, uint16_t maxValue) {
    int32_t adjusted = value + delta;
    if (adjusted < minValue) {
        value = minValue;
    } else if (adjusted > maxValue) {
        value = maxValue;
    } else {
        value = adjusted;
    }
}

// The flag rows share toggle/edit behavior; action rows are handled separately.
bool itemIsFlag(uint8_t item) {
    return item >= ITEM_DIRTY && item <= ITEM_SLEEPING;
}

// Map menu row ids back to the PetStatus bit they control.
PetStatus itemStatus(uint8_t item) {
    switch (item)
    {
    case ITEM_DIRTY:
        return DIRTY;
    case ITEM_BORED:
        return BORED;
    case ITEM_ANXIOUS:
        return ANXIOUS;
    case ITEM_SCRATCHING:
        return SCRATCHING;
    case ITEM_SLEEPING:
        return SLEEPING;
    default:
        return DIRTY;
    }
}

// Apply button-only rows and flag toggles.
void applySelected() {
    if (itemIsFlag(selectedItem)) {
        PetStatus status = itemStatus(selectedItem);
        setFlag(status, !petHas(status));
        return;
    }

    switch (selectedItem)
    {
    case ITEM_CLEAR_FLAGS:
        clearStatuses();
        break;
    case ITEM_RESET_SAVE:
        resetSave();
        break;
    default:
        break;
    }
}

// Left/right editing path; holding B uses larger steps for counters.
void adjustSelected(int8_t direction, bool largeStep) {
    if (itemIsFlag(selectedItem)) {
        setFlag(itemStatus(selectedItem), direction > 0);
        return;
    }

    switch (selectedItem)
    {
    case ITEM_LIFE:
        adjustUint8(pet.life, direction * (largeStep ? 2 : 1), 1, MAX_LIFE);
        savePet();
        break;
    case ITEM_MONEY:
        adjustUint16(pet.money, direction * (largeStep ? 100 : 10), 0, 9999);
        savePet();
        break;
    case ITEM_LEVEL:
        adjustUint8(pet.level, direction * (largeStep ? 10 : 1), 0, 99);
        savePet();
        break;
    case ITEM_XP:
        adjustUint8(pet.xpTenths, direction * (largeStep ? 5 : 1), 0, 9);
        clampXp();
        savePet();
        break;
    default:
        break;
    }
}

// Draws the selection marker without consuming horizontal room in each row.
void printCursor(Tinyfont &tinyfont, uint8_t item) {
    tinyfont.setCursor(0, 6 + item * 5);
    tinyfont.print(selectedItem == item ? ">" : " ");
}

// Boolean row renderer for pet status flags.
void printBool(Tinyfont &tinyfont, uint8_t item, const char *label, bool enabled) {
    printCursor(tinyfont, item);
    tinyfont.print(label);
    tinyfont.setCursor(72, 6 + item * 5);
    tinyfont.print(enabled ? "1" : "0");
}

// Numeric row renderer for counters edited in place.
void printCounter(Tinyfont &tinyfont, uint8_t item, const char *label, uint16_t value) {
    printCursor(tinyfont, item);
    tinyfont.print(label);
    tinyfont.setCursor(72, 6 + item * 5);
    tinyfont.print(value);
}

// Life row also shows whether the derived HUNGRY condition is active.
void printLife(Tinyfont &tinyfont) {
    printCursor(tinyfont, ITEM_LIFE);
    tinyfont.print("hp");
    tinyfont.setCursor(72, 6 + ITEM_LIFE * 5);
    tinyfont.print(pet.life);
    tinyfont.print("/");
    tinyfont.print(MAX_LIFE);
    tinyfont.print(petHas(HUNGRY) ? " H" : "");
}

// Command rows have labels but no right-side value.
void printAction(Tinyfont &tinyfont, uint8_t item, const char *label) {
    printCursor(tinyfont, item);
    tinyfont.print(label);
}

// Full-screen debug overlay; kept dense to fit every test control at once.
void drawMenu(Arduboy2Base &arduboy, Tinyfont &tinyfont) {
    printLife(tinyfont);
    printCounter(tinyfont, ITEM_MONEY, "$", pet.money);
    printCounter(tinyfont, ITEM_LEVEL, "lv", pet.level);
    printCounter(tinyfont, ITEM_XP, "xp", pet.xpTenths);
    printBool(tinyfont, ITEM_DIRTY, "di", petHas(DIRTY));
    printBool(tinyfont, ITEM_BORED, "bo", petHas(BORED));
    printBool(tinyfont, ITEM_ANXIOUS, "an", petHas(ANXIOUS));
    printBool(tinyfont, ITEM_SCRATCHING, "sc", petHas(SCRATCHING));
    printBool(tinyfont, ITEM_SLEEPING, "sl", petHas(SLEEPING));
    printAction(tinyfont, ITEM_CLEAR_FLAGS, "clr");
    printAction(tinyfont, ITEM_RESET_SAVE, "rst");

}
}

// Returns true when debug consumed the frame and the game scene should not draw.
bool debugHandleFrame(Arduboy2Base &arduboy, Tinyfont &tinyfont) {
    if (arduboy.pressed(A_BUTTON) && arduboy.pressed(B_BUTTON) && arduboy.justPressed(UP_BUTTON)) {
        visible = !visible;
        return true;
    }

    if (!visible) {
        return false;
    }

    if (arduboy.justPressed(A_BUTTON)) {
        visible = false;
        return true;
    }
    if (arduboy.justPressed(DOWN_BUTTON) && selectedItem + 1 < ITEM_COUNT) {
        selectedItem++;
    }
    if (arduboy.justPressed(UP_BUTTON) && selectedItem > 0) {
        selectedItem--;
    }
    if (arduboy.justPressed(LEFT_BUTTON)) {
        adjustSelected(-1, arduboy.pressed(B_BUTTON));
    }
    if (arduboy.justPressed(RIGHT_BUTTON)) {
        adjustSelected(1, arduboy.pressed(B_BUTTON));
    }
    if (arduboy.justPressed(B_BUTTON)) {
        applySelected();
    }

    drawMenu(arduboy, tinyfont);
    return true;
}

#else

// Stable builds compile this no-op so Game.cpp does not need debug #ifdefs.
bool debugHandleFrame(Arduboy2Base &, Tinyfont &) {
    return false;
}

#endif
