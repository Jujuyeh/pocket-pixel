#include "PocketPixelGame.h"

#include "Pet.h"
#include "Personality.h"
#include "Save.h"
#include "Assets.h"
#include "Render.h"
#include "Debug.h"
#include "Link.h"

#include <Arduboy2.h>
#include <Tinyfont.h>
#include <ArduboyTones.h>
 
Arduboy2Base arduboy;
Tinyfont tinyfont = Tinyfont(arduboy.sBuffer, Arduboy2Base::width(), Arduboy2Base::height());
ArduboyTones sound(arduboy.audio.enabled);
uint16_t frameCounter = 0;

void dirty(int16_t dx);
void scratching();
void updateScratchSirenLed();
void closeEyes(int16_t dx = 0);
void clearZZZ();
uint8_t idleBreathFrames();
void playMenuMoveTone();

/************************************ GAME ************************************/

enum GAME_STATE {
  IDLE,
  FEED,
  PLAY,
  CLEAN,
  WALK,
  WATER,
  VISIT_MENU,
  AIR_HOCKEY
} state = IDLE;

enum MenuAction : uint8_t {
  ACTION_FEED,
  ACTION_PLAY,
  ACTION_CLEAN,
  ACTION_WALK,
  ACTION_WATER,
};

void setState(GAME_STATE status);
void finishCareAction(PetStatus status, uint8_t xpReward, uint16_t moneyReward);

namespace {
constexpr uint8_t GAME_FPS = 30;
constexpr uint8_t BASE_FPS = 10;
constexpr uint8_t FPS_SCALE = GAME_FPS / BASE_FPS;
constexpr uint8_t framesAtGameFps(uint8_t baseFrames) {
    return baseFrames * FPS_SCALE;
}

constexpr uint8_t LITTER_X = 4;
constexpr uint8_t LITTER_Y = 3;
constexpr uint8_t LITTER_W = 120;
constexpr uint8_t LITTER_H = 52;
constexpr uint8_t LITTER_MIN_X = LITTER_X + 7;
constexpr uint8_t LITTER_MAX_X = LITTER_X + LITTER_W + 1;
constexpr uint8_t LITTER_MIN_Y = LITTER_Y + 4;
constexpr uint8_t LITTER_MAX_Y = LITTER_Y + LITTER_H - 5;
constexpr uint8_t LITTER_UI_Y = 56;
constexpr uint8_t POOP_COUNT = 4;
constexpr uint8_t POOP_WIDTH = 8;
constexpr uint8_t POOP_HEIGHT = 6;
constexpr int8_t CLEAN_TOOL_STEP = 1;
constexpr uint8_t TOOL_BACKUP_W = 22;
constexpr uint8_t TOOL_BACKUP_PAGES = 2;
constexpr int8_t TOOL_BRUSH_OFFSET_X = -3;
constexpr int8_t CLEAN_BRUSH_LEFT = -5;
constexpr int8_t CLEAN_BRUSH_RIGHT = 1;
constexpr int8_t CLEAN_BRUSH_TOP = -4;
constexpr int8_t CLEAN_BRUSH_BOTTOM = 4;
constexpr uint16_t CLEAN_BASE_REWARD = 10;
constexpr uint8_t PET_IDLE_DRAW_X = 26;
constexpr uint8_t PET_IDLE_DRAW_Y = 21;
constexpr uint8_t PET_IDLE_WIDTH = 26;
constexpr uint8_t PET_PLAY_DRAW_Y = 19;
constexpr uint8_t FEED_ITEM_COUNT = 3;
constexpr uint8_t FEED_ITEM_W = 12;
constexpr uint8_t FEED_ITEM_H = 12;
constexpr uint8_t FEED_POOP_W = 16;
constexpr uint8_t FEED_POOP_H = 16;
constexpr uint8_t FEED_COIN_W = 8;
constexpr uint8_t FEED_COIN_H = 8;
constexpr uint8_t FEED_COIN_REWARD = 50;
constexpr uint8_t FEED_FISH_COST = 50;
constexpr uint8_t FEED_CHICKEN_COST = 20;
constexpr uint8_t FEED_BOWL_W = 16;
constexpr uint8_t FEED_BOWL_Y = 42;
constexpr uint8_t FEED_BOWL_MIN_X = 2;
constexpr uint8_t FEED_BOWL_MAX_X = 110;
constexpr uint8_t FEED_BOWL_FRAC_BITS = 8;
constexpr uint16_t FEED_BOWL_UNIT = 1 << FEED_BOWL_FRAC_BITS;
constexpr uint16_t FEED_BOWL_STEP_SUBPX = (4U * FEED_BOWL_UNIT * BASE_FPS) / GAME_FPS;
constexpr uint8_t FEED_SPAWN_INTERVAL = 7;
constexpr uint8_t FEED_ITEM_FRAC_BITS = 2;
constexpr int16_t FEED_ITEM_UNIT = 1 << FEED_ITEM_FRAC_BITS;
constexpr int16_t FEED_FALL_STEP_SUBPX = 3;
constexpr uint8_t FEED_FULL_UNITS = MAX_LIFE * 2;
constexpr uint8_t PLAY_SCORE_REQUIRED = 6;
constexpr uint8_t PLAY_CAT_ANCHOR_X = 33;
constexpr uint8_t PLAY_CAT_ANCHOR_Y = 47;
constexpr uint8_t PLAY_HAND_X = 67;
constexpr uint8_t PLAY_HAND_Y = 24;
constexpr uint8_t PLAY_HAND_RETRACT_FRAMES = 3;
constexpr uint8_t PLAY_HAND_HOLD_FRAMES = framesAtGameFps(4);
constexpr uint8_t PLAY_HAND_RETURN_FRAMES = 4;
constexpr uint8_t PLAY_FEEDBACK_FRAMES = framesAtGameFps(8);
constexpr uint8_t PLAY_SCRATCH_FLASH_FRAMES = framesAtGameFps(4);
constexpr uint8_t WATER_PROGRESS_MAX = 72;
constexpr uint8_t WATER_PROGRESS_STEP = 4;
constexpr uint8_t WATER_DRAIN_INTERVAL = 2;
constexpr uint8_t WATER_TARGET_MIN_FRAMES = framesAtGameFps(12);
constexpr uint8_t WATER_TARGET_MAX_FRAMES = framesAtGameFps(24);
constexpr uint8_t WATER_SPRAY_FEEDBACK_FRAMES = framesAtGameFps(3);
constexpr uint8_t WATER_SPRAY_X = 73;
constexpr uint8_t WATER_SPRAY_Y = 26;
constexpr uint8_t WATER_SPRAY_NOZZLE_X = 0;
constexpr uint8_t WATER_SPRAY_NOZZLE_Y = 3;
constexpr uint8_t WATER_BAR_X = 12;
constexpr uint8_t WATER_BAR_Y = 56;
constexpr uint8_t WATER_BAR_W = 72;
constexpr uint8_t WATER_BAR_H = 6;
constexpr uint8_t WALK_SCORE_REQUIRED = 16;
constexpr uint8_t WALK_OBSTACLE_COUNT = 3;
constexpr uint8_t WALK_CAT_X = 28;
constexpr uint8_t WALK_CAT_W = 16;
constexpr uint8_t WALK_CAT_H = 12;
constexpr uint8_t WALK_CAT_BORDER_W = 18;
constexpr uint8_t WALK_OBSTACLE_W = 12;
constexpr uint8_t WALK_OBSTACLE_H = 12;
constexpr uint8_t WALK_COIN_W = 8;
constexpr uint8_t WALK_COIN_H = 8;
constexpr uint8_t WALK_COIN_REWARD = 50;
constexpr uint8_t WALK_SUBPX_BITS = 4;
constexpr int16_t WALK_SUBPX_UNIT = 1 << WALK_SUBPX_BITS;
constexpr int16_t WALK_LASER_STEP_SUBPX = 16;
constexpr int16_t WALK_CAT_STEP_SUBPX = 12;
constexpr int16_t WALK_OBSTACLE_SPEED_SUBPX = 24;
constexpr uint8_t WALK_LASER_X = 86;
constexpr uint8_t WALK_EMITTER_X = 117;
constexpr uint8_t WALK_EMITTER_Y = 34;
constexpr uint8_t WALK_LANE_DASH_W = 8;
constexpr uint8_t WALK_SPAWN_MIN_FRAMES = framesAtGameFps(7);
constexpr uint8_t WALK_SPAWN_MAX_FRAMES = framesAtGameFps(13);
constexpr uint8_t WALK_HIT_FLASH_FRAMES = framesAtGameFps(4);
constexpr uint8_t WALK_COIN_FEEDBACK_FRAMES = framesAtGameFps(8);
#ifdef POCKET_PIXEL_FXC_LINK
constexpr uint8_t HOCKEY_SCORE_REQUIRED = 4;
constexpr uint8_t HOCKEY_SUBPX_BITS = 4;
constexpr int16_t HOCKEY_UNIT = 1 << HOCKEY_SUBPX_BITS;
constexpr uint8_t HOCKEY_PUCK_R = 2;
constexpr uint8_t HOCKEY_CAT_BOTTOM_Y = 63;
constexpr uint8_t HOCKEY_CAT_REST_FRAME = 0;
constexpr uint8_t HOCKEY_CAT_STRIKE_FRAMES = framesAtGameFps(5);
constexpr uint8_t HOCKEY_ROUND_PAUSE_FRAMES = framesAtGameFps(18);
constexpr uint8_t HOCKEY_FLASH_FRAMES = framesAtGameFps(4);
constexpr uint8_t HOCKEY_EXIT_HOLD_FRAMES = framesAtGameFps(30);
constexpr uint8_t HOCKEY_MIN_SPEED_Y = 17;
constexpr uint8_t HOCKEY_MAX_SPEED_Y = 34;
constexpr uint8_t HOCKEY_PADDLE_Y_HOST = 114;
constexpr uint8_t HOCKEY_PADDLE_Y_CLIENT = 13;
constexpr uint8_t VISIT_IDLE_SPRITE_SIZE = 2 + PET_IDLE_WIDTH * 3;
constexpr uint8_t VISIT_LEAVE_FRAMES = framesAtGameFps(10);
constexpr uint8_t VISIT_PAUSE_FRAMES = framesAtGameFps(6);
constexpr uint8_t VISIT_SHUTTER_FRAMES = framesAtGameFps(14);
#endif
constexpr uint8_t BOOT_LOGO_X = 52;
constexpr uint8_t BOOT_LOGO_Y = 26;
constexpr uint8_t BOOT_DUST_START_FRAMES = framesAtGameFps(20);
constexpr uint8_t BOOT_FILL_START_FRAMES = framesAtGameFps(32);
constexpr uint8_t BOOT_CURTAIN_START_FRAMES = framesAtGameFps(47);
constexpr uint8_t BOOT_TOTAL_FRAMES = framesAtGameFps(52);

const uint8_t WALK_LANE_Y[3] = {18, 34, 50};

enum CleanTool : uint8_t {
    CLEAN_TOOL_RAKE,
    CLEAN_TOOL_SCOOP,
};

enum FeedItemType : uint8_t {
    FEED_ITEM_FISH,
    FEED_ITEM_CHICKEN,
    FEED_ITEM_POOP,
    FEED_ITEM_COIN,
};

enum PlayCatPhase : uint8_t {
    PLAY_CAT_IDLE,
    PLAY_CAT_WARNING,
    PLAY_CAT_LUNGE,
    PLAY_CAT_STRIKE,
    PLAY_CAT_RECOVER,
};

enum PlayFeedback : uint8_t {
    PLAY_FEEDBACK_NONE,
    PLAY_FEEDBACK_SCORE,
    PLAY_FEEDBACK_SCRATCH,
};

enum PlayHandPhase : uint8_t {
    PLAY_HAND_OUT,
    PLAY_HAND_RETRACTING,
    PLAY_HAND_HOLD,
    PLAY_HAND_RETURNING,
};

enum WaterTargetButton : uint8_t {
    WATER_TARGET_UP,
    WATER_TARGET_RIGHT,
    WATER_TARGET_DOWN,
    WATER_TARGET_LEFT,
};

enum WalkObstacleType : uint8_t {
    WALK_OBSTACLE_BOX,
    WALK_OBSTACLE_PUDDLE,
    WALK_OBSTACLE_ROCK,
};

struct FeedItem {
    bool active = false;
    uint8_t x = 0;
    int16_t ySubpx = 0;
    FeedItemType type = FEED_ITEM_FISH;
};

struct FeedMinigame {
    bool initialized = false;
    uint16_t bowlXSubpx = 56U * FEED_BOWL_UNIT;
    uint8_t initialUnits = 0;
    uint8_t foodUnits = 0;
    uint8_t spawnCooldown = 0;
    uint8_t flashFrames = 0;
    uint8_t coinFeedbackFrames = 0;
    FeedItem items[FEED_ITEM_COUNT];
};

struct PlayMinigame {
    bool initialized = false;
    PlayCatPhase catPhase = PLAY_CAT_IDLE;
    PlayHandPhase handPhase = PLAY_HAND_OUT;
    PlayFeedback feedback = PLAY_FEEDBACK_NONE;
    uint8_t phaseFrames = 0;
    uint8_t idleTargetFrames = framesAtGameFps(3);
    uint8_t handPhaseFrames = 0;
    uint8_t feedbackFrames = 0;
    uint8_t flashFrames = 0;
    uint8_t score = 0;
};

struct WaterMinigame {
    bool initialized = false;
    uint8_t progress = 0;
    WaterTargetButton target = WATER_TARGET_UP;
    uint8_t targetFrames = WATER_TARGET_MIN_FRAMES;
    uint8_t sprayFrames = 0;
};

struct WalkObstacle {
    bool active = false;
    bool scored = false;
    bool coin = false;
    int16_t xSubpx = 0;
    uint8_t lane = 1;
    WalkObstacleType type = WALK_OBSTACLE_BOX;
};

struct WalkMinigame {
    bool initialized = false;
    int16_t laserYSubpx = WALK_LANE_Y[1] * WALK_SUBPX_UNIT;
    int16_t catYSubpx = WALK_LANE_Y[1] * WALK_SUBPX_UNIT;
    uint8_t spawnCooldown = WALK_SPAWN_MIN_FRAMES;
    uint16_t laneScrollSubpx = 0;
    uint8_t runFrame = 0;
    uint8_t score = 0;
    uint8_t flashFrames = 0;
    uint8_t coinFeedbackFrames = 0;
    bool collectedMoney = false;
    WalkObstacle obstacles[WALK_OBSTACLE_COUNT];
};

#ifdef POCKET_PIXEL_FXC_LINK
struct AirHockeyMinigame {
    bool initialized = false;
    bool host = true;
    uint8_t localX = 48;
    uint8_t remoteX = 48;
    bool localStrike = false;
    bool remoteStrike = false;
    uint8_t localStrikeFrames = 0;
    uint8_t hostScore = 0;
    uint8_t clientScore = 0;
    int16_t puckXSubpx = 64 * HOCKEY_UNIT;
    int16_t puckYSubpx = 64 * HOCKEY_UNIT;
    int8_t puckVx = 3;
    int8_t puckVy = 18;
    uint8_t roundPauseFrames = HOCKEY_ROUND_PAUSE_FRAMES;
    uint8_t flashFrames = 0;
    uint8_t scoreFlashFrames = HOCKEY_ROUND_PAUSE_FRAMES;
    uint8_t exitHoldFrames = 0;
};

struct VisitTransfer {
    bool active = false;
    bool profileSent = false;
    uint8_t sprite = LINK_VISIT_SPRITE_IDLE1;
    uint8_t chunk = 0;
};

struct RemoteVisitData {
    LinkVisitProfile profile = {};
    uint8_t idleSprites[2][VISIT_IDLE_SPRITE_SIZE] = {};
    bool spriteReady[2] = {};
};

struct VisitMenuState {
    uint8_t frame = 0;
    uint8_t selected = 0;
};

struct VisitHostState {
    bool active = false;
    uint8_t frame = 0;
};
#endif

const uint8_t POOP_SHAPES[3][POOP_HEIGHT] = {
    {0b00000000, 0b01111000, 0b11111100, 0b11111110, 0b01111100, 0b00000000},
    {0b00000000, 0b00111100, 0b11111110, 0b11111100, 0b01111000, 0b00000000},
    {0b00000000, 0b01111100, 0b11111110, 0b01111110, 0b00111100, 0b00000000},
};

struct PoopPatch {
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t shape = 0;
    uint8_t revealed[POOP_HEIGHT] = {};
    uint8_t cleared[POOP_HEIGHT] = {};
};

enum CleanFlag : uint8_t {
    CLEAN_INITIALIZED = 1 << 0,
    CLEAN_HAS_HIDDEN_COIN = 1 << 1,
    CLEAN_COIN_FOUND = 1 << 2,
    CLEAN_COIN_COLLECTED = 1 << 3,
};

struct CleanMinigame {
    uint8_t flags = 0;
    uint8_t cursorX = 64;
    uint8_t cursorY = 28;
    CleanTool tool = CLEAN_TOOL_RAKE;
    PoopPatch poops[POOP_COUNT];
    uint8_t coinX = 0;
    uint8_t coinY = 0;
    uint16_t coinValue = 0;
    uint16_t bonusMoney = 0;
    uint8_t feedbackFrames = 0;
};

CleanMinigame cleanGame;
FeedMinigame feedGame;
PlayMinigame playGame;
WaterMinigame waterGame;
WalkMinigame walkGame;
#ifdef POCKET_PIXEL_FXC_LINK
AirHockeyMinigame hockeyGame;
VisitTransfer visitTransfer;
RemoteVisitData remoteVisit;
VisitMenuState visitMenuState;
VisitHostState visitHostState;
#endif

struct ToolOverlay {
    bool valid = false;
    uint8_t x = 0;
    uint8_t page = 0;
    uint8_t width = 0;
    uint8_t pages = 0;
    uint8_t bytes[TOOL_BACKUP_W * TOOL_BACKUP_PAGES] = {};
};

ToolOverlay toolOverlay;

// Clean minigame flags are packed to avoid spending separate bytes on AVR RAM.
bool cleanFlag(CleanFlag flag) {
    return (cleanGame.flags & flag) != 0;
}

void setCleanFlag(CleanFlag flag, bool enabled) {
    if (enabled) {
        cleanGame.flags |= flag;
    } else {
        cleanGame.flags &= ~flag;
    }
}

// Cursor bounds are based on the brush tip, not the whole tool sprite.
uint8_t clampToLitterX(int8_t delta) {
    int16_t moved = cleanGame.cursorX + delta;
    if (moved < LITTER_MIN_X) {
        return LITTER_MIN_X;
    }
    if (moved > LITTER_MAX_X) {
        return LITTER_MAX_X;
    }
    return moved;
}

// Vertical clamp mirrors clampToLitterX() for the active brush tip.
uint8_t clampToLitterY(int8_t delta) {
    int16_t moved = cleanGame.cursorY + delta;
    if (moved < LITTER_MIN_Y) {
        return LITTER_MIN_Y;
    }
    if (moved > LITTER_MAX_Y) {
        return LITTER_MAX_Y;
    }
    return moved;
}

// Hidden litter coins are weighted toward small rewards.
uint16_t randomCoinValue() {
    uint8_t roll = random(0, 100);
    if (roll < 60) {
        return 100;
    }
    if (roll < 85) {
        return 200;
    }
    if (roll < 97) {
        return 500;
    }
    return 1000;
}

// Deterministic sand noise; avoids storing a sand framebuffer in RAM.
uint8_t sandHash(uint8_t x, uint8_t y, uint8_t salt) {
    return (uint8_t) ((x * 17U + y * 29U + salt * 37U) & 0xFF);
}

// Circular reveal brush used for initial partial poop visibility.
bool pointInBrush(uint8_t px, uint8_t py, uint8_t brushX, uint8_t brushY, uint8_t radius) {
    int8_t dx = (int8_t) px - (int8_t) brushX;
    int8_t dy = (int8_t) py - (int8_t) brushY;
    return dx * dx + dy * dy <= radius * radius;
}

// Shaped clean brush: a slightly rounded rectangle aligned to the tool head.
bool pointInCleanBrush(uint8_t px, uint8_t py, uint8_t brushX, uint8_t brushY) {
    int8_t dx = (int8_t) px - (int8_t) brushX;
    int8_t dy = (int8_t) py - (int8_t) brushY;
    if (dx < CLEAN_BRUSH_LEFT || dx > CLEAN_BRUSH_RIGHT
        || dy < CLEAN_BRUSH_TOP || dy > CLEAN_BRUSH_BOTTOM) {
        return false;
    }
    if (dy == CLEAN_BRUSH_TOP || dy == CLEAN_BRUSH_BOTTOM) {
        return dx > CLEAN_BRUSH_LEFT && dx < CLEAN_BRUSH_RIGHT;
    }
    return true;
}

uint8_t cleanBrushX() {
    return cleanGame.cursorX + TOOL_BRUSH_OFFSET_X;
}

uint8_t cleanBrushY() {
    return cleanGame.cursorY;
}

void drawRoundedLitterFrame(Arduboy2Base &display) {
    display.drawFastHLine(LITTER_X + 1, LITTER_Y, LITTER_W - 2, BLACK);
    display.drawFastHLine(LITTER_X + 1, LITTER_Y + LITTER_H - 1, LITTER_W - 2, BLACK);
    display.drawFastVLine(LITTER_X, LITTER_Y + 1, LITTER_H - 2, BLACK);
    display.drawFastVLine(LITTER_X + LITTER_W - 1, LITTER_Y + 1, LITTER_H - 2, BLACK);
}

// Draw one deterministic sand speck when its hash passes the density threshold.
void drawSandPoint(Arduboy2Base &display, uint8_t x, uint8_t y, uint8_t salt) {
    if (sandHash(x / 3, y / 3, salt) % 5 < 2) {
        display.drawPixel(x, y, BLACK);
    }
}

// Initial litter texture is procedural so it costs no persistent framebuffer.
void drawBaseSand(Arduboy2Base &display) {
    for (uint8_t y = LITTER_Y + 3; y < LITTER_Y + LITTER_H - 3; y += 3) {
        for (uint8_t x = LITTER_X + 3; x < LITTER_X + LITTER_W - 3; x += 4) {
            uint8_t px = x + (sandHash(x, y, 1) % 3);
            uint8_t py = y + (sandHash(x, y, 2) % 3);
            drawSandPoint(display, px, py, 0);
        }
    }
}

// Raking repaints a small local patch with new sparse sand dots.
void drawRakedSandDot(Arduboy2Base &display, uint8_t brushX, uint8_t brushY, uint8_t index) {
    int8_t x = (int8_t) (sandHash(brushX + index * 3, brushY, index) % 7) - 5;
    int8_t y = (int8_t) (sandHash(brushY + index * 5, brushX, index + 11) % 9) - 4;
    uint8_t px = brushX + x;
    uint8_t py = brushY + y;
    if (px > LITTER_X && px < LITTER_X + LITTER_W - 1
        && py > LITTER_Y && py < LITTER_Y + LITTER_H - 1
        && pointInCleanBrush(px, py, brushX, brushY)) {
        display.drawPixel(px, py, BLACK);
    }
}

// White-out then re-dot the brush footprint so the rake visibly moves sand.
void drawRakedSandInBrush(Arduboy2Base &display, uint8_t brushX, uint8_t brushY) {
    for (int8_t y = CLEAN_BRUSH_TOP; y <= CLEAN_BRUSH_BOTTOM; y++) {
        for (int8_t x = CLEAN_BRUSH_LEFT; x <= CLEAN_BRUSH_RIGHT; x++) {
            uint8_t px = brushX + x;
            uint8_t py = brushY + y;
            if (px > LITTER_X && px < LITTER_X + LITTER_W - 1
                && py > LITTER_Y && py < LITTER_Y + LITTER_H - 1
                && pointInCleanBrush(px, py, brushX, brushY)) {
                display.drawPixel(px, py, WHITE);
            }
        }
    }
    for (uint8_t i = 0; i < 4; i++) {
        drawRakedSandDot(display, brushX, brushY, i);
    }
}

// Poop shapes are stored as MSB-first row bitmasks.
bool shapeBit(uint8_t shape, uint8_t row, uint8_t col) {
    return (POOP_SHAPES[shape][row] & (0x80 >> col)) != 0;
}

// Draw a single poop shape pixel in screen coordinates.
void drawPoopPixel(Arduboy2Base &display, PoopPatch &poop, uint8_t row, uint8_t col, uint8_t color) {
    if (shapeBit(poop.shape, row, col)) {
        display.drawPixel(poop.x + col, poop.y + row, color);
    }
}

// A visible poop pixel must be part of the shape, revealed, and not scooped.
bool visiblePoopPixel(const PoopPatch &poop, uint8_t row, uint8_t col) {
    uint8_t bit = 0x80 >> col;
    return (poop.revealed[row] & bit) != 0
        && (poop.cleared[row] & bit) == 0
        && shapeBit(poop.shape, row, col);
}

// White outline improves readability when poop patches overlap.
void drawPoopBorderPixel(Arduboy2Base &display, const PoopPatch &poop, int8_t row, int8_t col) {
    int16_t x = poop.x + col;
    int16_t y = poop.y + row;
    if (x <= LITTER_X || x >= LITTER_X + LITTER_W - 1 || y <= LITTER_Y || y >= LITTER_Y + LITTER_H - 1) {
        return;
    }
    display.drawPixel(x, y, WHITE);
}

// Draw borders first so the black poop body can sit on top.
void drawVisiblePoopBorders(Arduboy2Base &display) {
    for (uint8_t i = 0; i < POOP_COUNT; i++) {
        PoopPatch &poop = cleanGame.poops[i];
        for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
            for (uint8_t col = 0; col < POOP_WIDTH; col++) {
                if (!visiblePoopPixel(poop, row, col)) {
                    continue;
                }
                drawPoopBorderPixel(display, poop, row - 1, col);
                drawPoopBorderPixel(display, poop, row + 1, col);
                drawPoopBorderPixel(display, poop, row, col - 1);
                drawPoopBorderPixel(display, poop, row, col + 1);
            }
        }
    }
}

// Render all currently revealed and uncleared poop pixels.
void drawVisiblePoops(Arduboy2Base &display) {
    drawVisiblePoopBorders(display);
    for (uint8_t i = 0; i < POOP_COUNT; i++) {
        PoopPatch &poop = cleanGame.poops[i];
        for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
            for (uint8_t col = 0; col < POOP_WIDTH; col++) {
                if (visiblePoopPixel(poop, row, col)) {
                    drawPoopPixel(display, poop, row, col, BLACK);
                }
            }
        }
    }
}

// Rake mode reveals poop but never clears it.
void revealPoopsInBrush(uint8_t brushX, uint8_t brushY) {
    for (uint8_t i = 0; i < POOP_COUNT; i++) {
        PoopPatch &poop = cleanGame.poops[i];
        for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
            for (uint8_t col = 0; col < POOP_WIDTH; col++) {
                if (shapeBit(poop.shape, row, col)
                    && pointInCleanBrush(poop.x + col, poop.y + row, brushX, brushY)) {
                    poop.revealed[row] |= 0x80 >> col;
                }
            }
        }
    }
}

// Initial reveal uses a rounder brush so some poop starts partially visible.
void revealPoopPatch(PoopPatch &poop, uint8_t brushX, uint8_t brushY, uint8_t radius) {
    for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
        for (uint8_t col = 0; col < POOP_WIDTH; col++) {
            if (shapeBit(poop.shape, row, col)
                && pointInBrush(poop.x + col, poop.y + row, brushX, brushY, radius)) {
                poop.revealed[row] |= 0x80 >> col;
            }
        }
    }
}

// Scoop mode clears only pixels that were already revealed.
void scoopPoopsInBrush(Arduboy2Base &display, uint8_t brushX, uint8_t brushY) {
    for (uint8_t i = 0; i < POOP_COUNT; i++) {
        PoopPatch &poop = cleanGame.poops[i];
        for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
            for (uint8_t col = 0; col < POOP_WIDTH; col++) {
                uint8_t bit = 0x80 >> col;
                if ((poop.revealed[row] & bit) != 0
                    && (poop.cleared[row] & bit) == 0
                    && pointInCleanBrush(poop.x + col, poop.y + row, brushX, brushY)) {
                    poop.cleared[row] |= bit;
                    drawPoopPixel(display, poop, row, col, WHITE);
                }
            }
        }
    }
}

// Completion is pixel-accurate: the entire shape must be scooped.
bool poopFullyCleared(PoopPatch &poop) {
    for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
        uint8_t shapeRow = POOP_SHAPES[poop.shape][row];
        if ((poop.cleared[row] & shapeRow) != shapeRow) {
            return false;
        }
    }
    return true;
}

// UI counter helper for the bottom-left clean minigame readout.
uint8_t collectedPoopCount() {
    uint8_t collected = 0;
    for (uint8_t i = 0; i < POOP_COUNT; i++) {
        if (poopFullyCleared(cleanGame.poops[i])) {
            collected++;
        }
    }
    return collected;
}

// Rake reveals the hidden coin; scoop collects it later.
void revealCoinInBrush(uint8_t brushX, uint8_t brushY) {
    if (!cleanFlag(CLEAN_HAS_HIDDEN_COIN) || cleanFlag(CLEAN_COIN_FOUND)) {
        return;
    }
    if (pointInCleanBrush(cleanGame.coinX, cleanGame.coinY, brushX, brushY)) {
        setCleanFlag(CLEAN_COIN_FOUND, true);
    }
}

// Shared 8x8-ish coin glyph used by clean/feed/walk rewards.
void drawCoin(Arduboy2Base &display, uint8_t x, uint8_t y, uint8_t color) {
    display.drawCircle(x, y, 3, color);
    display.drawFastVLine(x, y - 2, 5, color);
    display.drawPixel(x - 1, y - 1, color);
    display.drawPixel(x + 1, y + 1, color);
}

// Coins require the scoop to collect so rake-only play cannot auto-claim money.
void collectCoinInBrush(Arduboy2Base &display, uint8_t brushX, uint8_t brushY) {
    if (!cleanFlag(CLEAN_HAS_HIDDEN_COIN)
        || !cleanFlag(CLEAN_COIN_FOUND)
        || cleanFlag(CLEAN_COIN_COLLECTED)) {
        return;
    }
    if (pointInCleanBrush(cleanGame.coinX, cleanGame.coinY, brushX, brushY)) {
        setCleanFlag(CLEAN_COIN_COLLECTED, true);
        drawCoin(display, cleanGame.coinX, cleanGame.coinY, WHITE);
        cleanGame.bonusMoney += cleanGame.coinValue;
        cleanGame.feedbackFrames = framesAtGameFps(18);
        sound.tones(coinPickup);
    }
}

// Leave the sand framebuffer alone until the next clean scene initialization.
void resetCleanMinigame() {
    setCleanFlag(CLEAN_INITIALIZED, false);
    toolOverlay.valid = false;
}

// Initialize clean directly into the Arduboy screen buffer; it is the scene state.
void initCleanMinigame() {
    cleanGame.flags = 0;
    setCleanFlag(CLEAN_INITIALIZED, true);
    cleanGame.cursorX = 64;
    cleanGame.cursorY = 29;
    cleanGame.tool = CLEAN_TOOL_RAKE;
    setCleanFlag(CLEAN_HAS_HIDDEN_COIN, random(0, 100) < 35);
    cleanGame.coinX = random(LITTER_MIN_X + 6, LITTER_MAX_X - 6);
    cleanGame.coinY = random(LITTER_MIN_Y + 6, LITTER_MAX_Y - 6);
    cleanGame.coinValue = randomCoinValue();
    cleanGame.bonusMoney = 0;
    cleanGame.feedbackFrames = 0;

    for (uint8_t i = 0; i < POOP_COUNT; i++) {
        PoopPatch &poop = cleanGame.poops[i];
        poop.x = random(LITTER_MIN_X + 2, LITTER_MAX_X - POOP_WIDTH);
        poop.y = random(LITTER_MIN_Y + 2, LITTER_MAX_Y - POOP_HEIGHT);
        poop.shape = random(0, 3);
        for (uint8_t row = 0; row < POOP_HEIGHT; row++) {
            poop.revealed[row] = 0;
            poop.cleared[row] = 0;
        }
    }

    for (uint8_t i = 0; i < random(1, 4); i++) {
        PoopPatch &poop = cleanGame.poops[i];
        revealPoopPatch(
            poop,
            poop.x + 2 + random(0, 4),
            poop.y + 2 + random(0, 3),
            3);
    }

    arduboy.fillScreen(WHITE);
    drawRoundedLitterFrame(arduboy);
    drawBaseSand(arduboy);
    drawVisiblePoops(arduboy);
    arduboy.display();
}

// Clean ends only when every hidden/revealed poop patch is fully scooped.
bool cleanComplete() {
    return collectedPoopCount() == POOP_COUNT;
}

// Applies base reward plus optional hidden coin bonus.
void finishCleanMinigame() {
    finishCareAction(DIRTY, 1, CLEAN_BASE_REWARD + cleanGame.bonusMoney);
    resetCleanMinigame();
}

// Tool handles may extend offscreen; clip to avoid drawing outside buffer bounds.
uint8_t clippedToolHandleLength(uint8_t x, int8_t offset, uint8_t desired) {
    int16_t start = x + offset;
    if (start < 0) {
        return 0;
    }
    if (start >= 127) {
        return 0;
    }
    uint8_t available = 127 - start;
    return desired < available ? desired : available;
}

// Draw only the tool overlay; the underlying litter state is restored separately.
void drawToolOverlay(Arduboy2Base &display, uint8_t x, uint8_t y) {
    if (cleanGame.tool == CLEAN_TOOL_RAKE) {
        display.drawFastVLine(x - 4, y - 4, 9, BLACK);
        for (int8_t tooth = -3; tooth <= 3; tooth += 2) {
            display.drawFastHLine(x - 7, y + tooth, 4, BLACK);
        }
        uint8_t handleLength = clippedToolHandleLength(x, -3, 14);
        if (handleLength > 0) {
            display.drawFastHLine(x - 3, y, handleLength, BLACK);
        }
    } else {
        display.drawFastVLine(x - 5, y - 4, 9, BLACK);
        display.drawFastHLine(x - 5, y - 4, 7, BLACK);
        display.drawFastHLine(x - 5, y + 4, 7, BLACK);
        display.drawPixel(x + 2, y - 3, BLACK);
        display.drawPixel(x + 3, y - 2, BLACK);
        display.drawPixel(x + 3, y + 2, BLACK);
        display.drawPixel(x + 2, y + 3, BLACK);
        display.drawFastHLine(x - 1, y, 6, BLACK);
        uint8_t handleLength = clippedToolHandleLength(x, 4, 10);
        if (handleLength > 0) {
            display.drawFastHLine(x + 4, y, handleLength, BLACK);
        }
    }
}

// Restore the screen bytes saved before the previous tool overlay was drawn.
void restoreToolOverlay(Arduboy2Base &display) {
    if (!toolOverlay.valid) {
        return;
    }
    for (uint8_t page = 0; page < toolOverlay.pages; page++) {
        for (uint8_t x = 0; x < toolOverlay.width; x++) {
            display.sBuffer[toolOverlay.x + x + (toolOverlay.page + page) * 128] =
                toolOverlay.bytes[x + page * TOOL_BACKUP_W];
        }
    }
    toolOverlay.valid = false;
}

// Save a small byte-window around the tool so drawing the overlay is reversible.
void saveToolOverlay(Arduboy2Base &display, uint8_t toolX, uint8_t toolY) {
    uint8_t left = toolX > 8 ? toolX - 8 : 0;
    uint8_t right = toolX > 114 ? 127 : toolX + 13;
    uint8_t top = toolY > 4 ? toolY - 4 : 0;
    uint8_t bottom = toolY + 4 > 63 ? 63 : toolY + 4;
    uint8_t startPage = top / 8;
    uint8_t endPage = bottom / 8;

    toolOverlay.valid = true;
    toolOverlay.x = left;
    toolOverlay.page = startPage;
    toolOverlay.width = right - left + 1;
    toolOverlay.pages = endPage - startPage + 1;
    if (toolOverlay.width > TOOL_BACKUP_W) {
        toolOverlay.width = TOOL_BACKUP_W;
    }
    if (toolOverlay.pages > TOOL_BACKUP_PAGES) {
        toolOverlay.pages = TOOL_BACKUP_PAGES;
    }

    for (uint8_t page = 0; page < toolOverlay.pages; page++) {
        for (uint8_t x = 0; x < toolOverlay.width; x++) {
            toolOverlay.bytes[x + page * TOOL_BACKUP_W] =
                display.sBuffer[toolOverlay.x + x + (toolOverlay.page + page) * 128];
        }
    }
}

// Clean does not clear the full screen each frame; it redraws only UI and overlay.
void drawCleanMinigame(Arduboy2Base &display, Tinyfont &font) {
    display.fillRect(0, LITTER_UI_Y, 128, 8, WHITE);
    font.setCursor(2, 58);
    font.print(collectedPoopCount());
    font.print("/");
    font.print(POOP_COUNT);

    if (cleanGame.feedbackFrames > 0 && cleanFlag(CLEAN_COIN_FOUND)) {
        font.setCursor(53, 58);
        font.print("+");
        font.print(cleanGame.coinValue);
    }

    font.setCursor(98, 58);
    font.print(cleanGame.tool == CLEAN_TOOL_RAKE ? "RAKE" : "SCOOP");

    if (cleanFlag(CLEAN_COIN_FOUND) && !cleanFlag(CLEAN_COIN_COLLECTED)) {
        drawCoin(display, cleanGame.coinX, cleanGame.coinY, BLACK);
    }
    drawVisiblePoops(display);
    saveToolOverlay(display, cleanGame.cursorX, cleanGame.cursorY);
    drawToolOverlay(display, cleanGame.cursorX, cleanGame.cursorY);
}
}

void resetFeedMinigame() {
    feedGame = FeedMinigame();
}

// Reset helpers rely on default member initializers so scene state stays compact.
void resetPlayMinigame() {
    playGame = PlayMinigame();
}

void resetWaterMinigame() {
    waterGame = WaterMinigame();
}

void resetWalkMinigame() {
    walkGame = WalkMinigame();
}

#ifdef POCKET_PIXEL_FXC_LINK
void resetAirHockeyMinigame() {
    hockeyGame = AirHockeyMinigame();
}

bool linkIdleActive() {
    return state == IDLE && linkPeerAvailable();
}

LinkVisitProfile localVisitProfile() {
    LinkVisitProfile profile = {};
    profile.breathFrames = idleBreathFrames();
    profile.playfulness = ActivePersonality.playfulness;
    profile.fishPreference = ActivePersonality.fishPreference;
    profile.chickenPreference = ActivePersonality.chickenPreference;
    return profile;
}

const uint8_t *visitSpriteData(uint8_t sprite) {
    return sprite == LINK_VISIT_SPRITE_IDLE2 ? PET_IDLE2 : PET_IDLE1;
}

void startVisitTransfer() {
    visitTransfer = VisitTransfer();
    visitTransfer.active = true;
}

void clearRemoteVisit() {
    remoteVisit = RemoteVisitData();
}

void startVisitHost() {
    clearRemoteVisit();
    visitHostState = VisitHostState();
    visitHostState.active = true;
}

void stopVisitHost() {
    visitHostState = VisitHostState();
}

void startVisitMenu() {
    visitMenuState = VisitMenuState();
    setState(VISIT_MENU);
}

void updateVisitTransfer() {
    LinkVisitProfile profile;
    if (linkConsumeVisitProfile(profile)) {
        remoteVisit.profile = profile;
    }

    LinkSpriteChunk receivedChunk;
    if (linkConsumeSpriteChunk(receivedChunk)
        && receivedChunk.sprite <= LINK_VISIT_SPRITE_IDLE2
        && receivedChunk.chunk <= VISIT_IDLE_SPRITE_SIZE - LINK_SPRITE_CHUNK_BYTES) {
        for (uint8_t i = 0; i < LINK_SPRITE_CHUNK_BYTES; i++) {
            remoteVisit.idleSprites[receivedChunk.sprite][receivedChunk.chunk + i] = receivedChunk.data[i];
        }
        if (receivedChunk.chunk + LINK_SPRITE_CHUNK_BYTES >= VISIT_IDLE_SPRITE_SIZE) {
            remoteVisit.spriteReady[receivedChunk.sprite] = true;
        }
    }

    if (!visitTransfer.active || !linkPeerAvailable()) {
        return;
    }

    if (!visitTransfer.profileSent) {
        if (linkSendVisitProfile(localVisitProfile())) {
            visitTransfer.profileSent = true;
        }
        return;
    }

    if (visitTransfer.sprite > LINK_VISIT_SPRITE_IDLE2) {
        visitTransfer.active = false;
        return;
    }

    LinkSpriteChunk chunk = {};
    chunk.sprite = visitTransfer.sprite;
    chunk.chunk = visitTransfer.chunk;
    const uint8_t *spriteData = visitSpriteData(chunk.sprite);
    for (uint8_t i = 0; i < LINK_SPRITE_CHUNK_BYTES; i++) {
        chunk.data[i] = pgm_read_byte(spriteData + chunk.chunk + i);
    }
    if (!linkSendSpriteChunk(chunk)) {
        return;
    }

    visitTransfer.chunk += LINK_SPRITE_CHUNK_BYTES;
    if (visitTransfer.chunk >= VISIT_IDLE_SPRITE_SIZE) {
        visitTransfer.chunk = 0;
        visitTransfer.sprite++;
    }
}
#endif

uint8_t feedBowlX() {
    return feedGame.bowlXSubpx >> FEED_BOWL_FRAC_BITS;
}

// Falling food uses low-resolution subpixels to keep movement smooth at 30 FPS.
int8_t feedItemY(const FeedItem &item) {
    return item.ySubpx / FEED_ITEM_UNIT;
}

// Food taste is profile-driven; equal or missing weights fall back to fair random.
FeedItemType randomFoodType() {
    uint16_t total = ActivePersonality.fishPreference + ActivePersonality.chickenPreference;
    if (total == 0) {
        return random(0, 2) == 0 ? FEED_ITEM_FISH : FEED_ITEM_CHICKEN;
    }
    return random(0, total) < ActivePersonality.fishPreference ? FEED_ITEM_FISH : FEED_ITEM_CHICKEN;
}

uint8_t idleBreathFrames() {
    if (ActivePersonality.anxiety <= 15) {
        return framesAtGameFps(5);
    }
    uint8_t fasterSteps = (ActivePersonality.anxiety - 15) / 25;
    uint8_t frames = framesAtGameFps(5);
    return fasterSteps >= 5 ? framesAtGameFps(3) : frames - fasterSteps;
}

uint8_t feedItemWidth(FeedItemType type) {
    switch (type)
    {
    case FEED_ITEM_POOP:
        return FEED_POOP_W;
    case FEED_ITEM_COIN:
        return FEED_COIN_W;
    case FEED_ITEM_FISH:
    case FEED_ITEM_CHICKEN:
    default:
        return FEED_ITEM_W;
    }
}

uint8_t feedItemHeight(FeedItemType type) {
    switch (type)
    {
    case FEED_ITEM_POOP:
        return FEED_POOP_H;
    case FEED_ITEM_COIN:
        return FEED_COIN_H;
    case FEED_ITEM_FISH:
    case FEED_ITEM_CHICKEN:
    default:
        return FEED_ITEM_H;
    }
}

FeedItemType randomFeedItemType() {
    if (random(0, pet.money < 100 ? 4 : 10) == 0) {
        return FEED_ITEM_COIN;
    }
    if (random(0, 7) == 0) {
        return FEED_ITEM_POOP;
    }
    return randomFoodType();
}

// Higher playfulness shortens the wait before Pixel decides to swat.
uint8_t randomPlayIdleDuration() {
    uint8_t restlessness = 100 - ActivePersonality.playfulness;
    uint8_t minFrames = framesAtGameFps(1) + (static_cast<uint16_t>(restlessness) * framesAtGameFps(2)) / 100;
    uint8_t maxFrames = framesAtGameFps(2) + (static_cast<uint16_t>(restlessness) * framesAtGameFps(4)) / 100;
    if (maxFrames <= minFrames) {
        maxFrames = minFrames + framesAtGameFps(1);
    }
    return random(minFrames, maxFrames + 1);
}

WaterTargetButton randomWaterTarget() {
    return static_cast<WaterTargetButton>(random(0, 4));
}

uint8_t randomWaterTargetDuration() {
    return random(WATER_TARGET_MIN_FRAMES, WATER_TARGET_MAX_FRAMES + 1);
}

uint8_t randomWalkSpawnDuration() {
    return random(WALK_SPAWN_MIN_FRAMES, WALK_SPAWN_MAX_FRAMES + 1);
}

void initFeedMinigame() {
    resetFeedMinigame();
    feedGame.initialized = true;
    feedGame.bowlXSubpx = 56U * FEED_BOWL_UNIT;
    feedGame.initialUnits = pet.life * 2;
    feedGame.foodUnits = feedGame.initialUnits;
    feedGame.spawnCooldown = framesAtGameFps(2);
}

void initPlayMinigame() {
    resetPlayMinigame();
    playGame.initialized = true;
    playGame.idleTargetFrames = randomPlayIdleDuration();
}

void initWaterMinigame() {
    resetWaterMinigame();
    waterGame.initialized = true;
    waterGame.target = randomWaterTarget();
    waterGame.targetFrames = randomWaterTargetDuration();
}

void initWalkMinigame() {
    resetWalkMinigame();
    walkGame.initialized = true;
    walkGame.spawnCooldown = framesAtGameFps(6);
}

#ifdef POCKET_PIXEL_FXC_LINK
uint8_t hockeyMobilityStep() {
    return 2 + (100 - ActivePersonality.playfulness) / 40;
}

uint8_t hockeyReachBonus() {
    return 2 + ActivePersonality.playfulness / 35;
}

uint8_t hockeyPowerBonus() {
    return 2 + ActivePersonality.playfulness / 30;
}

uint8_t hockeyCatFrame(bool striking, uint8_t frames) {
    if (!striking) {
        return HOCKEY_CAT_REST_FRAME;
    }
    if (frames < framesAtGameFps(2)) {
        return 1;
    }
    return 2;
}

const uint8_t *hockeyCatSprite(bool striking, uint8_t frames) {
    switch (hockeyCatFrame(striking, frames))
    {
    case 1:
        return playCatPaw_1;
    case 2:
        return playCatPaw_2;
    case 0:
    default:
        return playCatPaw_0;
    }
}

const uint8_t *hockeyCatMask(bool striking, uint8_t frames) {
    switch (hockeyCatFrame(striking, frames))
    {
    case 1:
        return playCatPawMask_1;
    case 2:
        return playCatPawMask_2;
    case 0:
    default:
        return playCatPawMask_0;
    }
}

uint8_t hockeyCatWidth(bool striking, uint8_t frames) {
    return pgm_read_byte(hockeyCatSprite(striking, frames));
}

void resetHockeyRound(bool hostServes) {
    hockeyGame.puckXSubpx = (56 + random(0, 17)) * HOCKEY_UNIT;
    hockeyGame.puckYSubpx = 64 * HOCKEY_UNIT;
    hockeyGame.puckVx = random(0, 2) == 0 ? -5 : 5;
    hockeyGame.puckVy = hostServes ? HOCKEY_MIN_SPEED_Y : -HOCKEY_MIN_SPEED_Y;
    hockeyGame.roundPauseFrames = HOCKEY_ROUND_PAUSE_FRAMES;
    hockeyGame.scoreFlashFrames = HOCKEY_ROUND_PAUSE_FRAMES;
}

void initAirHockeyMinigame() {
    resetAirHockeyMinigame();
    hockeyGame.initialized = true;
    hockeyGame.host = linkLocalIsHost();
    hockeyGame.localX = 48;
    hockeyGame.remoteX = 48;
    if (hockeyGame.host) {
        resetHockeyRound(random(0, 2) == 0);
    }
}

void finishAirHockeyMinigame() {
    addXpTenths(3);
    hurtPet(2);
    sound.tones(feedDone);
    saveGame();
    resetAirHockeyMinigame();
    arduboy.invert(false);
    setState(IDLE);
}

void updateHockeyLocalInput() {
    hockeyGame.localStrike = arduboy.pressed(B_BUTTON);
    if (hockeyGame.localStrike) {
        if (hockeyGame.localStrikeFrames < HOCKEY_CAT_STRIKE_FRAMES) {
            hockeyGame.localStrikeFrames++;
        }
        return;
    }

    hockeyGame.localStrikeFrames = 0;
    uint8_t step = hockeyMobilityStep();
    if (arduboy.pressed(LEFT_BUTTON) && hockeyGame.localX > step) {
        hockeyGame.localX -= step;
    }
    if (arduboy.pressed(RIGHT_BUTTON)) {
        uint8_t w = hockeyCatWidth(false, 0);
        uint8_t maxX = 127 - w;
        hockeyGame.localX = hockeyGame.localX + step > maxX ? maxX : hockeyGame.localX + step;
    }
}

bool hockeyPuckHitsPaddle(uint8_t paddleX, bool striking, bool hostSide) {
    if (!striking) {
        return false;
    }
    uint8_t width = hockeyCatWidth(true, HOCKEY_CAT_STRIKE_FRAMES);
    int16_t puckX = hockeyGame.puckXSubpx / HOCKEY_UNIT;
    int16_t puckY = hockeyGame.puckYSubpx / HOCKEY_UNIT;
    uint8_t paddleY = hostSide ? HOCKEY_PADDLE_Y_HOST : HOCKEY_PADDLE_Y_CLIENT;
    return puckX + HOCKEY_PUCK_R >= paddleX
        && puckX <= static_cast<int16_t>(paddleX) + width
        && abs(puckY - paddleY) <= 4 + hockeyReachBonus();
}

uint8_t hockeyRemoteGlobalX(const LinkInput &remoteInput) {
    uint8_t width = hockeyCatWidth(remoteInput.strike, HOCKEY_CAT_STRIKE_FRAMES);
    return 127 - constrain(static_cast<int16_t>(remoteInput.x) + width, 0, 127);
}

void hockeyBounceFromPaddle(uint8_t paddleX, bool hostSide) {
    uint8_t width = hockeyCatWidth(true, HOCKEY_CAT_STRIKE_FRAMES);
    int16_t puckX = hockeyGame.puckXSubpx / HOCKEY_UNIT;
    int16_t center = paddleX + width / 2;
    int16_t offset = puckX - center;
    int8_t power = HOCKEY_MIN_SPEED_Y + hockeyPowerBonus();
    hockeyGame.puckVx = constrain(offset, -18, 18);
    hockeyGame.puckVy = hostSide ? -power : power;
    sound.tones(feedCatch);
}

void hockeyPoint(bool hostScored) {
    if (hostScored) {
        hockeyGame.hostScore++;
    } else {
        hockeyGame.clientScore++;
    }
    hockeyGame.flashFrames = HOCKEY_FLASH_FRAMES;
    sound.tones(feedBad);
    resetHockeyRound(hostScored);
}

void updateHockeyHostPhysics() {
    LinkInput remoteInput;
    if (linkConsumeInput(remoteInput)) {
        hockeyGame.remoteX = hockeyRemoteGlobalX(remoteInput);
        hockeyGame.remoteStrike = remoteInput.strike;
    }

    if (hockeyGame.roundPauseFrames > 0) {
        hockeyGame.roundPauseFrames--;
        return;
    }

    hockeyGame.puckXSubpx += hockeyGame.puckVx;
    hockeyGame.puckYSubpx += hockeyGame.puckVy;

    int16_t puckX = hockeyGame.puckXSubpx / HOCKEY_UNIT;
    if (puckX <= HOCKEY_PUCK_R || puckX >= 127 - HOCKEY_PUCK_R) {
        hockeyGame.puckVx = -hockeyGame.puckVx;
        hockeyGame.puckXSubpx = constrain(hockeyGame.puckXSubpx, HOCKEY_PUCK_R * HOCKEY_UNIT, (127 - HOCKEY_PUCK_R) * HOCKEY_UNIT);
    }

    if (hockeyGame.puckVy > 0
        && hockeyPuckHitsPaddle(hockeyGame.localX, hockeyGame.localStrike, true)) {
        hockeyBounceFromPaddle(hockeyGame.localX, true);
    } else if (hockeyGame.puckVy < 0
        && hockeyPuckHitsPaddle(hockeyGame.remoteX, hockeyGame.remoteStrike, false)) {
        hockeyBounceFromPaddle(hockeyGame.remoteX, false);
    }

    int16_t puckY = hockeyGame.puckYSubpx / HOCKEY_UNIT;
    if (puckY > 127 + HOCKEY_PUCK_R) {
        hockeyPoint(false);
    } else if (puckY < -HOCKEY_PUCK_R) {
        hockeyPoint(true);
    }

    if (abs(hockeyGame.puckVy) < HOCKEY_MIN_SPEED_Y) {
        hockeyGame.puckVy = hockeyGame.puckVy < 0 ? -HOCKEY_MIN_SPEED_Y : HOCKEY_MIN_SPEED_Y;
    }
    hockeyGame.puckVy = constrain(hockeyGame.puckVy, -HOCKEY_MAX_SPEED_Y, HOCKEY_MAX_SPEED_Y);
}

void syncHockeyState() {
    if (hockeyGame.host) {
        LinkState statePacket = {};
        statePacket.hostX = hockeyGame.localX;
        statePacket.clientX = hockeyGame.remoteX;
        statePacket.hostScore = hockeyGame.hostScore;
        statePacket.clientScore = hockeyGame.clientScore;
        statePacket.puckX = constrain(hockeyGame.puckXSubpx / HOCKEY_UNIT, 0, 127);
        statePacket.puckY = constrain(hockeyGame.puckYSubpx / HOCKEY_UNIT, 0, 127);
        statePacket.flags = (hockeyGame.roundPauseFrames > 0 ? 1 : 0)
            | (hockeyGame.flashFrames > 0 ? 2 : 0);
        linkSendState(statePacket);
    } else {
        linkSendInput(hockeyGame.localX, hockeyGame.localStrike);
        LinkState statePacket;
        if (linkConsumeState(statePacket)) {
            hockeyGame.remoteX = statePacket.hostX;
            hockeyGame.hostScore = statePacket.hostScore;
            hockeyGame.clientScore = statePacket.clientScore;
            hockeyGame.puckXSubpx = statePacket.puckX * HOCKEY_UNIT;
            hockeyGame.puckYSubpx = statePacket.puckY * HOCKEY_UNIT;
            hockeyGame.roundPauseFrames = (statePacket.flags & 1) ? HOCKEY_ROUND_PAUSE_FRAMES : 0;
            hockeyGame.flashFrames = (statePacket.flags & 2) ? HOCKEY_FLASH_FRAMES : 0;
        }
    }
}

void updateAirHockeyMinigame() {
    updateHockeyLocalInput();
    if (hockeyGame.host) {
        updateHockeyHostPhysics();
    }
    if (hockeyGame.flashFrames > 0) {
        hockeyGame.flashFrames--;
    }
    if (hockeyGame.scoreFlashFrames > 0) {
        hockeyGame.scoreFlashFrames--;
    }
    syncHockeyState();
}
#endif

// Food units are half-heart units; the bowl starts at the current saved hunger.
uint8_t feedLifeFromUnits() {
    uint8_t life = feedGame.foodUnits / 2;
    if (life < 1) {
        return 1;
    }
    return life > MAX_LIFE ? MAX_LIFE : life;
}

void applyFeedLifeProgress() {
    setPetLife(feedLifeFromUnits());
}

bool feedItemOverlapsBowl(const FeedItem &item) {
    uint8_t itemW = feedItemWidth(item.type);
    uint8_t bowlX = feedBowlX();
    return item.x + itemW > bowlX && item.x < bowlX + FEED_BOWL_W;
}

// Spawns into the first free slot so the game never allocates falling items.
void spawnFeedItem() {
    for (uint8_t i = 0; i < FEED_ITEM_COUNT; i++) {
        if (!feedGame.items[i].active) {
            FeedItem &item = feedGame.items[i];
            item.active = true;
            item.ySubpx = -12 * FEED_ITEM_UNIT;
            item.type = randomFeedItemType();
            uint8_t itemW = feedItemWidth(item.type);
            item.x = random(2, 126 - itemW);
            return;
        }
    }
}

void moveFeedBowl(int16_t delta) {
    int32_t moved = static_cast<int32_t>(feedGame.bowlXSubpx) + delta;
    int32_t minX = static_cast<int32_t>(FEED_BOWL_MIN_X) * FEED_BOWL_UNIT;
    int32_t maxX = static_cast<int32_t>(FEED_BOWL_MAX_X) * FEED_BOWL_UNIT;
    if (moved < minX) {
        moved = minX;
    }
    if (moved > maxX) {
        moved = maxX;
    }
    feedGame.bowlXSubpx = moved;
}

// Successful feed always finishes at full life regardless of partial progress.
void finishFeedMinigame() {
    setPetLife(MAX_LIFE);
    addXpTenths(1);
    sound.tones(feedDone);
    saveGame();
    setState(IDLE);
}

// Cat phase timing is authored in old 10 FPS units and scaled to GAME_FPS.
uint8_t playCatPhaseDuration(PlayCatPhase phase) {
    switch (phase)
    {
    case PLAY_CAT_IDLE:
        return playGame.idleTargetFrames;
    case PLAY_CAT_WARNING:
        return framesAtGameFps(6);
    case PLAY_CAT_LUNGE:
        return framesAtGameFps(2);
    case PLAY_CAT_STRIKE:
        return framesAtGameFps(2);
    case PLAY_CAT_RECOVER:
        return framesAtGameFps(5);
    default:
        return framesAtGameFps(8);
    }
}

// Play cat behavior is a fixed state machine: idle, warning, lunge, strike, recover.
PlayCatPhase nextPlayCatPhase(PlayCatPhase phase) {
    switch (phase)
    {
    case PLAY_CAT_IDLE:
        return PLAY_CAT_WARNING;
    case PLAY_CAT_WARNING:
        return PLAY_CAT_LUNGE;
    case PLAY_CAT_LUNGE:
        return PLAY_CAT_STRIKE;
    case PLAY_CAT_STRIKE:
        return PLAY_CAT_RECOVER;
    case PLAY_CAT_RECOVER:
    default:
        return PLAY_CAT_IDLE;
    }
}

// Sprite selection follows the same phase state machine as hit evaluation.
const uint8_t *playCatSprite() {
    switch (playGame.catPhase)
    {
    case PLAY_CAT_LUNGE:
    case PLAY_CAT_RECOVER:
        return playCatPaw_1;
    case PLAY_CAT_STRIKE:
        return playCatPaw_2;
    case PLAY_CAT_IDLE:
    case PLAY_CAT_WARNING:
    default:
        return playCatPaw_0;
    }
}

// Masks let differently sized cat frames sit over a drawn background cleanly.
const uint8_t *playCatMask() {
    switch (playGame.catPhase)
    {
    case PLAY_CAT_LUNGE:
    case PLAY_CAT_RECOVER:
        return playCatPawMask_1;
    case PLAY_CAT_STRIKE:
        return playCatPawMask_2;
    case PLAY_CAT_IDLE:
    case PLAY_CAT_WARNING:
    default:
        return playCatPawMask_0;
    }
}

// Idle/warning shakes are tiny offsets; the bottom-left anchor remains stable.
int8_t playCatOffsetX() {
    if (playGame.catPhase == PLAY_CAT_WARNING) {
        return ((playGame.phaseFrames / FPS_SCALE) & 1) == 0 ? -1 : 1;
    }
    if (playGame.catPhase == PLAY_CAT_IDLE) {
        return ((frameCounter / framesAtGameFps(10)) & 1) == 0 ? -1 : 1;
    }
    return 0;
}

// The hand teases while extended by looping 0,1,2,1.
uint8_t playHandFrame() {
    uint8_t phase = (frameCounter / framesAtGameFps(2)) % 4;
    switch (phase)
    {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    default:
        return 1;
    }
}

// Retraction is a short slide right; only the hold phase counts as safe.
uint8_t playHandX() {
    switch (playGame.handPhase)
    {
    case PLAY_HAND_RETRACTING:
        return PLAY_HAND_X + (static_cast<uint16_t>(playGame.handPhaseFrames) * 28) / PLAY_HAND_RETRACT_FRAMES;
    case PLAY_HAND_HOLD:
        return PLAY_HAND_X + 28;
    case PLAY_HAND_RETURNING:
        return PLAY_HAND_X + 28 - (static_cast<uint16_t>(playGame.handPhaseFrames) * 28) / PLAY_HAND_RETURN_FRAMES;
    case PLAY_HAND_OUT:
    default:
        return PLAY_HAND_X;
    }
}

bool playHandIsRetracted() {
    return playGame.handPhase == PLAY_HAND_HOLD;
}

uint8_t playHandPhaseDuration(PlayHandPhase phase) {
    switch (phase)
    {
    case PLAY_HAND_RETRACTING:
        return PLAY_HAND_RETRACT_FRAMES;
    case PLAY_HAND_HOLD:
        return PLAY_HAND_HOLD_FRAMES;
    case PLAY_HAND_RETURNING:
        return PLAY_HAND_RETURN_FRAMES;
    case PLAY_HAND_OUT:
    default:
        return 0;
    }
}

void advancePlayHandPhase() {
    if (playGame.handPhase == PLAY_HAND_OUT) {
        return;
    }

    playGame.handPhaseFrames++;
    if (playGame.handPhaseFrames < playHandPhaseDuration(playGame.handPhase)) {
        return;
    }

    playGame.handPhaseFrames = 0;
    switch (playGame.handPhase)
    {
    case PLAY_HAND_RETRACTING:
        playGame.handPhase = PLAY_HAND_HOLD;
        break;
    case PLAY_HAND_HOLD:
        playGame.handPhase = PLAY_HAND_RETURNING;
        break;
    case PLAY_HAND_RETURNING:
    default:
        playGame.handPhase = PLAY_HAND_OUT;
        break;
    }
}

// Strike scoring is deliberately frame-state based: retracted means success.
void applyPlayStrikeResult() {
    playGame.feedbackFrames = PLAY_FEEDBACK_FRAMES;
    if (!playHandIsRetracted()) {
        playGame.score = playGame.score > 3 ? playGame.score - 3 : 0;
        playGame.feedback = PLAY_FEEDBACK_SCRATCH;
        playGame.flashFrames = PLAY_SCRATCH_FLASH_FRAMES;
        sound.tones(feedBad);
    } else {
        playGame.feedback = PLAY_FEEDBACK_SCORE;
        if (playGame.score < PLAY_SCORE_REQUIRED) {
            playGame.score++;
        }
        sound.tones(feedCatch);
    }
}

// A strike is resolved exactly when the cat enters the strike frame.
void advancePlayCatPhase() {
    playGame.phaseFrames++;
    if (playGame.phaseFrames < playCatPhaseDuration(playGame.catPhase)) {
        return;
    }

    playGame.catPhase = nextPlayCatPhase(playGame.catPhase);
    playGame.phaseFrames = 0;
    if (playGame.catPhase == PLAY_CAT_STRIKE) {
        applyPlayStrikeResult();
    } else if (playGame.catPhase == PLAY_CAT_IDLE) {
        playGame.idleTargetFrames = randomPlayIdleDuration();
    }
}

void updatePlayMinigame() {
    if (arduboy.justPressed(B_BUTTON) && playGame.handPhase == PLAY_HAND_OUT) {
        playGame.handPhase = PLAY_HAND_RETRACTING;
        playGame.handPhaseFrames = 0;
    }

    advancePlayHandPhase();
    if (playGame.feedbackFrames > 0) {
        playGame.feedbackFrames--;
        if (playGame.feedbackFrames == 0) {
            playGame.feedback = PLAY_FEEDBACK_NONE;
        }
    }
    if (playGame.flashFrames > 0) {
        playGame.flashFrames--;
    }
    advancePlayCatPhase();
    if (playGame.score >= PLAY_SCORE_REQUIRED) {
        resetPlayMinigame();
        arduboy.invert(false);
        finishCareAction(BORED, 2, 5);
        return;
    }
}

// Arduboy sprites store width/height followed by vertical page bytes.
void drawSpriteBottomLeft(const uint8_t *sprite, int16_t x, int16_t bottomY) {
    uint8_t height = pgm_read_byte(sprite + 1);
    Sprites::drawOverwrite(x, bottomY - height + 1, sprite, 0);
}

// In Arduboy bitmaps a cleared bit is a black pixel.
bool spritePixelIsBlack(const uint8_t *sprite, uint8_t frame, uint8_t x, uint8_t y) {
    uint8_t width = pgm_read_byte(sprite);
    uint8_t height = pgm_read_byte(sprite + 1);
    uint8_t pages = (height + 7) / 8;
    uint16_t frameOffset = static_cast<uint16_t>(frame) * width * pages;
    uint8_t byte = pgm_read_byte(sprite + 2 + frameOffset + (y / 8) * width + x);
    return ((byte >> (y % 8)) & 1) == 0;
}

// Mask black pixels define which destination pixels may be overwritten.
void drawMaskedSprite(const uint8_t *sprite, const uint8_t *mask, uint8_t frame, int16_t x, int16_t y) {
    uint8_t width = pgm_read_byte(sprite);
    uint8_t height = pgm_read_byte(sprite + 1);
    for (uint8_t py = 0; py < height; py++) {
        for (uint8_t px = 0; px < width; px++) {
            if (spritePixelIsBlack(mask, frame, px, py)) {
                arduboy.drawPixel(x + px, y + py, spritePixelIsBlack(sprite, frame, px, py) ? BLACK : WHITE);
            }
        }
    }
}

void drawMaskedSpriteBottomLeft(const uint8_t *sprite, const uint8_t *mask, int16_t x, int16_t bottomY) {
    uint8_t height = pgm_read_byte(sprite + 1);
    drawMaskedSprite(sprite, mask, 0, x, bottomY - height + 1);
}

void drawSpritePixels(const uint8_t *sprite, uint8_t frame, int16_t x, int16_t y, uint8_t color) {
    uint8_t width = pgm_read_byte(sprite);
    uint8_t height = pgm_read_byte(sprite + 1);
    for (uint8_t py = 0; py < height; py++) {
        for (uint8_t px = 0; px < width; px++) {
            int16_t drawX = x + px;
            int16_t drawY = y + py;
            if (drawX >= 0 && drawX < 128 && drawY >= 0 && drawY < 64
                && spritePixelIsBlack(sprite, frame, px, py)) {
                arduboy.drawPixel(drawX, drawY, color);
            }
        }
    }
}

// Used by the boot logo: only source white pixels are painted over black.
void drawSpriteWhitePixels(const uint8_t *sprite, uint8_t frame, int16_t x, int16_t y) {
    uint8_t width = pgm_read_byte(sprite);
    uint8_t height = pgm_read_byte(sprite + 1);
    for (uint8_t py = 0; py < height; py++) {
        for (uint8_t px = 0; px < width; px++) {
            int16_t drawX = x + px;
            int16_t drawY = y + py;
            if (drawX >= 0 && drawX < 128 && drawY >= 0 && drawY < 64
                && !spritePixelIsBlack(sprite, frame, px, py)) {
                arduboy.drawPixel(drawX, drawY, WHITE);
            }
        }
    }
}

// Boot logo shake is procedural so no extra animation frames are stored.
void drawBootLogo(uint8_t frame) {
    int8_t dx = (frame % 4 == 0) ? -2 : (frame % 4 == 2) ? 2 : 0;
    int8_t dy = (frame % 6 == 1) ? -1 : (frame % 6 == 4) ? 1 : 0;
    drawSpriteWhitePixels(bootLogo24x12, 0, BOOT_LOGO_X + dx, BOOT_LOGO_Y + dy);
}

// Dust starts after the logo has established itself on screen.
void drawBootParticles(uint8_t frame) {
    for (uint8_t i = 0; i < 22; i++) {
        uint8_t fallSpeed = 1 + (i % 3);
        int16_t x = (i * 23 + frame / 2 + ((frame + i * 7) / 8) % 7) % 128;
        int16_t y = (i * 19 + frame * fallSpeed) % 72 - 8;
        if (y >= 0 && y < 64) {
            arduboy.drawPixel(x, y, WHITE);
        }
    }
}

// Fill accelerates in three phases: slow, medium, then fast.
uint16_t bootFillPixelCount(uint8_t frame) {
    uint16_t count = 0;
    for (uint8_t i = 0; i <= frame; i++) {
        if (i < framesAtGameFps(5)) {
            count += 10;
        } else if (i < framesAtGameFps(12)) {
            count += 34;
        } else {
            count += 420;
        }
    }
    return count;
}

// Cheap deterministic shuffle spreads fill pixels across the whole screen.
uint16_t bootFillPixelPosition(uint16_t index) {
    uint16_t value = index * 173U + 911U;
    value ^= value >> 7;
    value *= 197U;
    value ^= value >> 5;
    return value & 8191;
}

// Pixel fill writes in shuffled order instead of scanning rows.
void drawBootFillPixels(uint8_t frame) {
    uint16_t count = bootFillPixelCount(frame);
    if (count > 8192) {
        count = 8192;
    }

    for (uint16_t i = 0; i < count; i++) {
        uint16_t p = bootFillPixelPosition(i);
        arduboy.drawPixel(p & 127, p >> 7, WHITE);
    }
}

// Final curtain guarantees the transition ends on a fully white screen.
void drawBootCurtain(uint8_t frame) {
    uint8_t halfWidth = 3 + frame * 5 + (frame * frame) / 8;
    if (halfWidth > 64) {
        halfWidth = 64;
    }
    arduboy.fillRect(64 - halfWidth, 0, halfWidth * 2, 64, WHITE);
}

// Any button skips the custom boot animation, matching Arduboy boot UX.
void playBootAnimation() {
    for (uint8_t frame = 0; frame < BOOT_TOTAL_FRAMES; frame++) {
        while (!arduboy.nextFrame()) { }
        arduboy.pollButtons();
        if (arduboy.buttonsState()) {
            break;
        }

        arduboy.fillScreen(BLACK);
        drawBootLogo(frame);
        if (frame >= BOOT_DUST_START_FRAMES) {
            drawBootParticles(frame - BOOT_DUST_START_FRAMES);
        }
        if (frame >= BOOT_FILL_START_FRAMES) {
            drawBootFillPixels(frame - BOOT_FILL_START_FRAMES);
        }
        if (frame >= BOOT_CURTAIN_START_FRAMES) {
            drawBootCurtain(frame - BOOT_CURTAIN_START_FRAMES);
        }
        arduboy.display();
    }
    arduboy.fillScreen(WHITE);
    arduboy.display();
}

uint8_t walkLaneTargetY(uint8_t lane) {
    return WALK_LANE_Y[lane];
}

// Smoothly approaches a subpixel target without overshooting.
int16_t approachSubpx(int16_t current, int16_t target, int16_t step) {
    if (current < target) {
        int16_t moved = current + step;
        return moved > target ? target : moved;
    }
    if (current > target) {
        int16_t moved = current - step;
        return moved < target ? target : moved;
    }
    return current;
}

uint8_t walkCatBottomY() {
    return (walkGame.catYSubpx + (WALK_SUBPX_UNIT / 2)) >> WALK_SUBPX_BITS;
}

uint8_t walkLaserY() {
    return (walkGame.laserYSubpx + (WALK_SUBPX_UNIT / 2)) >> WALK_SUBPX_BITS;
}

uint8_t walkLaneScroll() {
    return walkGame.laneScrollSubpx >> WALK_SUBPX_BITS;
}

int16_t walkObstacleX(const WalkObstacle &obstacle) {
    return obstacle.xSubpx >> WALK_SUBPX_BITS;
}

// Obstacle type selects art only; collision rules are shared.
const uint8_t *walkObstacleSprite(WalkObstacleType type) {
    switch (type)
    {
    case WALK_OBSTACLE_PUDDLE:
        return walkObstaclePuddle;
    case WALK_OBSTACLE_ROCK:
        return walkObstacleRock;
    case WALK_OBSTACLE_BOX:
    default:
        return walkObstacleBox;
    }
}

// Coins reuse the obstacle slots because they scroll through lanes identically.
void spawnWalkObstacle() {
    for (uint8_t i = 0; i < WALK_OBSTACLE_COUNT; i++) {
        WalkObstacle &obstacle = walkGame.obstacles[i];
        if (obstacle.active) {
            continue;
        }
        obstacle.active = true;
        obstacle.scored = false;
        obstacle.coin = random(0, 4) == 0;
        obstacle.xSubpx = 128 * WALK_SUBPX_UNIT;
        obstacle.lane = random(0, 3);
        obstacle.type = static_cast<WalkObstacleType>(random(0, 3));
        return;
    }
}

// Coins use normal rectangle overlap so they feel generous to collect.
bool walkCoinHitsCat(const WalkObstacle &obstacle) {
    int16_t coinX = walkObstacleX(obstacle);
    uint8_t coinBottomY = walkLaneTargetY(obstacle.lane);
    uint8_t coinTopY = coinBottomY - WALK_COIN_H + 1;
    uint8_t catBottomY = walkCatBottomY();
    uint8_t catTopY = catBottomY - WALK_CAT_H + 1;
    return WALK_CAT_X + WALK_CAT_W > coinX
        && WALK_CAT_X < coinX + WALK_COIN_W
        && catBottomY >= coinTopY
        && catTopY <= coinBottomY;
}

// Obstacles use the cat's lower-right reference pixel against the lower half.
bool walkObstacleHitsCat(const WalkObstacle &obstacle) {
    int16_t obstacleX = walkObstacleX(obstacle);
    int16_t pointX = WALK_CAT_X + WALK_CAT_W - 1;
    uint8_t pointY = walkCatBottomY();
    uint8_t obstacleBottomY = walkLaneTargetY(obstacle.lane);
    uint8_t obstacleTopY = obstacleBottomY - WALK_OBSTACLE_H + 1;
    return pointX >= obstacleX
        && pointX < obstacleX + WALK_OBSTACLE_W
        && pointY >= obstacleTopY + (WALK_OBSTACLE_H / 2)
        && pointY <= obstacleBottomY;
}

void updateWalkMinigame() {
    // The laser is controlled directly; the cat follows it with a small delay.
    if (arduboy.pressed(UP_BUTTON)) {
        walkGame.laserYSubpx -= WALK_LASER_STEP_SUBPX;
    }
    if (arduboy.pressed(DOWN_BUTTON)) {
        walkGame.laserYSubpx += WALK_LASER_STEP_SUBPX;
    }
    int16_t laserMin = walkLaneTargetY(0) * WALK_SUBPX_UNIT;
    int16_t laserMax = walkLaneTargetY(2) * WALK_SUBPX_UNIT;
    if (walkGame.laserYSubpx < laserMin) {
        walkGame.laserYSubpx = laserMin;
    }
    if (walkGame.laserYSubpx > laserMax) {
        walkGame.laserYSubpx = laserMax;
    }
    walkGame.catYSubpx = approachSubpx(walkGame.catYSubpx, walkGame.laserYSubpx, WALK_CAT_STEP_SUBPX);

    if (arduboy.everyXFrames(framesAtGameFps(2))) {
        walkGame.runFrame = (walkGame.runFrame + 1) % 3;
    }
    // Lane dashes and objects share speed to sell forward movement.
    walkGame.laneScrollSubpx += WALK_OBSTACLE_SPEED_SUBPX;
    uint16_t lanePeriod = 12U * WALK_SUBPX_UNIT;
    while (walkGame.laneScrollSubpx >= lanePeriod) {
        walkGame.laneScrollSubpx -= lanePeriod;
    }

    if (walkGame.spawnCooldown > 0) {
        walkGame.spawnCooldown--;
    } else {
        spawnWalkObstacle();
        walkGame.spawnCooldown = randomWalkSpawnDuration();
    }

    for (uint8_t i = 0; i < WALK_OBSTACLE_COUNT; i++) {
        WalkObstacle &obstacle = walkGame.obstacles[i];
        if (!obstacle.active) {
            continue;
        }
        obstacle.xSubpx -= WALK_OBSTACLE_SPEED_SUBPX;
        if (obstacle.coin) {
            if (walkCoinHitsCat(obstacle)) {
                addMoney(WALK_COIN_REWARD);
                walkGame.collectedMoney = true;
                walkGame.coinFeedbackFrames = WALK_COIN_FEEDBACK_FRAMES;
                obstacle.active = false;
                sound.tones(coinPickup);
                continue;
            }
            if (walkObstacleX(obstacle) < -WALK_COIN_W) {
                obstacle.active = false;
            }
            continue;
        }
        if (!obstacle.scored && walkObstacleHitsCat(obstacle)) {
            // Hitting any obstacle resets progress but leaves the obstacle scrolling.
            walkGame.score = 0;
            walkGame.flashFrames = WALK_HIT_FLASH_FRAMES;
            obstacle.scored = true;
            sound.tones(feedBad);
            continue;
        }
        if (!obstacle.scored && walkObstacleX(obstacle) + WALK_OBSTACLE_W < WALK_CAT_X + WALK_CAT_W - 1) {
            // Score when the obstacle has fully passed the cat reference point.
            obstacle.scored = true;
            if (walkGame.score < WALK_SCORE_REQUIRED) {
                walkGame.score++;
            }
            sound.tones(feedCatch);
            continue;
        }
        if (walkObstacleX(obstacle) < -WALK_OBSTACLE_W) {
            obstacle.active = false;
        }
    }

    if (walkGame.flashFrames > 0) {
        walkGame.flashFrames--;
    }
    if (walkGame.coinFeedbackFrames > 0) {
        walkGame.coinFeedbackFrames--;
    }
}

// Dashes are clipped at both edges so they enter and leave smoothly.
void drawWalkLanes() {
    uint8_t scroll = walkLaneScroll();
    for (uint8_t lane = 0; lane < 3; lane++) {
        uint8_t y = walkLaneTargetY(lane);
        for (int16_t x = 128 - scroll; x > -WALK_LANE_DASH_W; x -= 12) {
            int16_t startX = x < 0 ? 0 : x;
            int16_t endX = x + WALK_LANE_DASH_W;
            if (endX > 128) {
                endX = 128;
            }
            if (endX > startX) {
                arduboy.drawFastHLine(startX, y + 1, endX - startX, BLACK);
            }
        }
    }
}

// The emitter and beam are drawn from one vector so they rotate together.
void drawWalkLaser() {
    uint8_t y = walkLaserY();
    constexpr int8_t VECTOR_SCALE = 8;
    constexpr uint8_t LASER_TARGET_X = WALK_LASER_X + 2;
    constexpr uint8_t DEVICE_LENGTH = 9;
    constexpr uint8_t DEVICE_HALF_WIDTH = 2;
    int16_t dy = static_cast<int16_t>(y) - WALK_EMITTER_Y;
    int8_t ux = -VECTOR_SCALE;
    int8_t uy = static_cast<int8_t>(dy / 4);
    int8_t px = -uy;
    int8_t py = ux;

    int16_t backX = WALK_EMITTER_X - ((ux * DEVICE_LENGTH) / VECTOR_SCALE);
    int16_t backY = WALK_EMITTER_Y - ((uy * DEVICE_LENGTH) / VECTOR_SCALE);
    int8_t offX = (px * DEVICE_HALF_WIDTH) / VECTOR_SCALE;
    int8_t offY = (py * DEVICE_HALF_WIDTH) / VECTOR_SCALE;

    arduboy.drawLine(WALK_EMITTER_X, WALK_EMITTER_Y, LASER_TARGET_X, y, BLACK);
    arduboy.drawLine(WALK_EMITTER_X + offX, WALK_EMITTER_Y + offY, backX + offX, backY + offY, BLACK);
    arduboy.drawLine(WALK_EMITTER_X - offX, WALK_EMITTER_Y - offY, backX - offX, backY - offY, BLACK);
    arduboy.drawLine(backX + offX, backY + offY, backX - offX, backY - offY, BLACK);
    arduboy.drawPixel(WALK_EMITTER_X, WALK_EMITTER_Y, BLACK);
    arduboy.drawPixel((WALK_EMITTER_X + backX) / 2 + offX, (WALK_EMITTER_Y + backY) / 2 + offY, BLACK);
    arduboy.drawPixel((WALK_EMITTER_X + backX) / 2 - offX, (WALK_EMITTER_Y + backY) / 2 - offY, BLACK);
    arduboy.drawFastHLine(WALK_LASER_X + 1, y, 3, BLACK);
    arduboy.drawFastVLine(WALK_LASER_X + 2, y - 1, 3, BLACK);
}

// White border is a separate mask-like sprite to keep black cat readable.
void drawWalkCat() {
    uint8_t bottomY = walkCatBottomY();
    uint8_t catY = bottomY - WALK_CAT_H + 1;
    drawSpritePixels(walkCatRunBorder, walkGame.runFrame, WALK_CAT_X - 1, catY - 1, WHITE);
    drawSpritePixels(walkCatRun, walkGame.runFrame, WALK_CAT_X, catY, BLACK);
}

void drawWalkObstacle(const WalkObstacle &obstacle) {
    int16_t x = walkObstacleX(obstacle);
    uint8_t y = walkLaneTargetY(obstacle.lane) - WALK_OBSTACLE_H + 1;
    drawSpritePixels(walkObstacleSprite(obstacle.type), 0, x, y, BLACK);
}

void drawWalkCoin(const WalkObstacle &obstacle) {
    int16_t x = walkObstacleX(obstacle);
    uint8_t y = walkLaneTargetY(obstacle.lane) - WALK_COIN_H + 1;
    arduboy.drawCircle(x + 3, y + 3, 3, BLACK);
    arduboy.drawFastVLine(x + 3, y + 1, 5, BLACK);
    arduboy.drawPixel(x + 5, y + 2, BLACK);
    arduboy.drawPixel(x + 5, y + 4, BLACK);
}

// Painter's order by bottom Y makes lower-lane objects appear in front.
void drawWalkWorldObjects() {
    uint8_t catBottomY = walkCatBottomY();
    bool catDrawn = false;
    for (uint8_t y = 0; y < 64; y++) {
        for (uint8_t i = 0; i < WALK_OBSTACLE_COUNT; i++) {
            const WalkObstacle &obstacle = walkGame.obstacles[i];
            if (obstacle.active && walkLaneTargetY(obstacle.lane) == y) {
                if (obstacle.coin) {
                    drawWalkCoin(obstacle);
                } else {
                    drawWalkObstacle(obstacle);
                }
            }
        }
        if (!catDrawn && catBottomY == y) {
            drawWalkCat();
            catDrawn = true;
        }
    }
    if (!catDrawn) {
        drawWalkCat();
    }
}

void drawWalkMinigame() {
    drawWalkLanes();
    drawWalkLaser();
    drawWalkWorldObjects();
    tinyfont.setCursor(2, 56);
    tinyfont.print("A EXIT");
    tinyfont.setCursor(96, 56);
    tinyfont.print(walkGame.score);
    tinyfont.print("/");
    tinyfont.print(WALK_SCORE_REQUIRED);
    if (walkGame.coinFeedbackFrames > 0) {
        tinyfont.setCursor(55, 7);
        tinyfont.print("+50");
    }

    bool invertFrame = walkGame.flashFrames > 0 && ((walkGame.flashFrames / FPS_SCALE) & 1) == 0;
    arduboy.invert(invertFrame);
}

// Feedback is intentionally minimal to avoid hiding the timing scene.
void drawPlayFeedback() {
    if (playGame.feedback == PLAY_FEEDBACK_SCORE) {
        tinyfont.setCursor(57, 7);
        tinyfont.print("+1");
    } else if (playGame.feedback == PLAY_FEEDBACK_SCRATCH) {
        tinyfont.setCursor(55, 7);
        tinyfont.print("-3");
    }
}

void drawPlayMinigame() {
#if defined(POCKET_PIXEL_FXC_LINK) || defined(POCKET_PIXEL_DEBUG)
    arduboy.drawRect(0, 0, 128, 54, BLACK);
    arduboy.drawFastVLine(64, 0, 54, BLACK);
#else
    Sprites::drawOverwrite(0, 0, playBackground, 0);
#endif
    uint8_t handFrame = playGame.handPhase == PLAY_HAND_OUT ? playHandFrame() : 0;
    drawMaskedSprite(playHand, playHandMask, handFrame, playHandX(), PLAY_HAND_Y);
    drawMaskedSpriteBottomLeft(playCatSprite(), playCatMask(), PLAY_CAT_ANCHOR_X + playCatOffsetX(), PLAY_CAT_ANCHOR_Y);
    drawPlayFeedback();

    tinyfont.setCursor(2, 55);
    tinyfont.print("A EXIT");
    tinyfont.setCursor(44, 55);
    tinyfont.print("B HAND");
    tinyfont.setCursor(91, 55);
    tinyfont.print(playGame.score);
    tinyfont.print("/");
    tinyfont.print(PLAY_SCORE_REQUIRED);

    bool invertFrame = playGame.flashFrames > 0 && ((playGame.flashFrames / FPS_SCALE) & 1) == 0;
    arduboy.invert(invertFrame);
}

#ifdef POCKET_PIXEL_FXC_LINK
int16_t hockeyScreenPuckY() {
    int16_t globalY = hockeyGame.puckYSubpx / HOCKEY_UNIT;
    return hockeyGame.host ? globalY - 64 : 63 - globalY;
}

int16_t hockeyScreenPuckX() {
    int16_t globalX = hockeyGame.puckXSubpx / HOCKEY_UNIT;
    return hockeyGame.host ? globalX : 127 - globalX;
}

uint8_t hockeyLocalScore() {
    return hockeyGame.host ? hockeyGame.hostScore : hockeyGame.clientScore;
}

uint8_t hockeyRemoteScore() {
    return hockeyGame.host ? hockeyGame.clientScore : hockeyGame.hostScore;
}

void drawHockeyRink() {
    arduboy.drawRect(0, 0, 128, 64, BLACK);
}

void drawHockeyCat(uint8_t x, uint8_t bottomY, bool strike, uint8_t strikeFrames) {
    drawMaskedSpriteBottomLeft(
        hockeyCatSprite(strike, strikeFrames),
        hockeyCatMask(strike, strikeFrames),
        x,
        bottomY);
}

void drawHockeyPuck() {
    int16_t puckX = hockeyScreenPuckX();
    int16_t puckY = hockeyScreenPuckY();
    if (puckY < -HOCKEY_PUCK_R || puckY > 63 + HOCKEY_PUCK_R) {
        return;
    }
    arduboy.fillCircle(puckX, puckY, HOCKEY_PUCK_R, BLACK);
}

void drawAirHockeyMinigame() {
    drawHockeyRink();
    drawHockeyPuck();
    drawHockeyCat(hockeyGame.localX, HOCKEY_CAT_BOTTOM_Y, hockeyGame.localStrike, hockeyGame.localStrikeFrames);

    bool showScore = hockeyGame.scoreFlashFrames == 0 || ((hockeyGame.scoreFlashFrames / FPS_SCALE) & 1) == 0;
    if (showScore) {
        tinyfont.setCursor(4, 4);
        tinyfont.print(hockeyLocalScore());
        tinyfont.print("-");
        tinyfont.print(hockeyRemoteScore());
    }
    bool invertFrame = hockeyGame.flashFrames > 0 && ((hockeyGame.flashFrames / FPS_SCALE) & 1) == 0;
    arduboy.invert(invertFrame);
}
#endif

void updateFeedMinigame() {
    if (arduboy.pressed(LEFT_BUTTON)) {
        moveFeedBowl(-static_cast<int16_t>(FEED_BOWL_STEP_SUBPX));
    }
    if (arduboy.pressed(RIGHT_BUTTON)) {
        moveFeedBowl(FEED_BOWL_STEP_SUBPX);
    }

    if (feedGame.spawnCooldown > 0) {
        feedGame.spawnCooldown--;
    } else {
        spawnFeedItem();
        feedGame.spawnCooldown = framesAtGameFps(FEED_SPAWN_INTERVAL + random(0, 4));
    }

    for (uint8_t i = 0; i < FEED_ITEM_COUNT; i++) {
        FeedItem &item = feedGame.items[i];
        if (!item.active) {
            continue;
        }

        item.ySubpx += FEED_FALL_STEP_SUBPX;
        int8_t itemY = feedItemY(item);
        uint8_t itemH = feedItemHeight(item.type);
        if (itemY + itemH >= FEED_BOWL_Y && itemY <= FEED_BOWL_Y + 4 && feedItemOverlapsBowl(item)) {
            if (item.type == FEED_ITEM_POOP) {
                // Bad drops halve both bowl fill and current life progress.
                feedGame.foodUnits /= 2;
                if (feedGame.foodUnits < 2) {
                    feedGame.foodUnits = 2;
                }
                feedGame.flashFrames = framesAtGameFps(4);
                sound.tones(feedBad);
            } else if (item.type == FEED_ITEM_COIN) {
                addMoney(FEED_COIN_REWARD);
                feedGame.coinFeedbackFrames = framesAtGameFps(8);
                sound.tones(coinPickup);
            } else if (feedGame.foodUnits < FEED_FULL_UNITS) {
                uint8_t cost = item.type == FEED_ITEM_FISH ? FEED_FISH_COST : FEED_CHICKEN_COST;
                if (pet.money < cost) {
                    feedGame.flashFrames = framesAtGameFps(4);
                    sound.tones(feedBad);
                    item.active = false;
                    continue;
                }
                takeMoney(cost);
                // Food increments by one half-heart unit.
                feedGame.foodUnits++;
                if (pet.money == 0) {
                    // Running out of money flashes like poop, but hunger progress stays.
                    feedGame.flashFrames = framesAtGameFps(4);
                    sound.tones(feedBad);
                } else {
                    sound.tones(feedCatch);
                }
            }
            item.active = false;
            continue;
        }

        if (itemY > 64) {
            item.active = false;
        }
    }

    if (feedGame.coinFeedbackFrames > 0) {
        feedGame.coinFeedbackFrames--;
    }
}

void drawFeedMinigame() {
    for (uint8_t i = 0; i < FEED_ITEM_COUNT; i++) {
        FeedItem &item = feedGame.items[i];
        int8_t itemY = feedItemY(item);
        if (!item.active || itemY < -15) {
            continue;
        }
        if (item.type == FEED_ITEM_POOP) {
            Sprites::drawOverwrite(item.x, itemY, poo1, 0);
        } else if (item.type == FEED_ITEM_COIN) {
            drawCoin(arduboy, item.x + 3, itemY + 3, BLACK);
        } else {
            Sprites::drawOverwrite(item.x, itemY, item.type == FEED_ITEM_FISH ? feedFish : feedChicken, 0);
        }
    }

    uint8_t fillFrame = feedLifeFromUnits();
    if (fillFrame > 5) {
        fillFrame = 5;
    }
    Sprites::drawOverwrite(feedBowlX(), FEED_BOWL_Y, feedBowl, fillFrame);
    tinyfont.setCursor(2, 55);
    tinyfont.print("A EXIT");
    tinyfont.setCursor(43, 55);
    tinyfont.print("NEED ");
    tinyfont.print(FEED_FULL_UNITS - feedGame.foodUnits);
    if (pet.money >= 100 || (arduboy.frameCount / framesAtGameFps(15)) % 2 == 0) {
        tinyfont.setCursor(95, 55);
        tinyfont.print("$");
        tinyfont.print(pet.money);
    }
    if (feedGame.coinFeedbackFrames > 0) {
        tinyfont.setCursor(56, 7);
        tinyfont.print("+50");
    }

    bool invertFrame = feedGame.flashFrames > 0 && ((feedGame.flashFrames / FPS_SCALE) & 1) == 0;
    arduboy.invert(invertFrame);
    if (feedGame.flashFrames > 0) {
        feedGame.flashFrames--;
    }
}

// Convert the target direction enum to Arduboy's button bitmask.
uint8_t waterButtonMask(WaterTargetButton target) {
    switch (target)
    {
    case WATER_TARGET_UP:
        return UP_BUTTON;
    case WATER_TARGET_RIGHT:
        return RIGHT_BUTTON;
    case WATER_TARGET_DOWN:
        return DOWN_BUTTON;
    case WATER_TARGET_LEFT:
    default:
        return LEFT_BUTTON;
    }
}

// Multiple direction buttons are treated as invalid input for the spam check.
uint8_t pressedWaterDirectionCount() {
    uint8_t count = 0;
    if (arduboy.pressed(UP_BUTTON)) count++;
    if (arduboy.pressed(RIGHT_BUTTON)) count++;
    if (arduboy.pressed(DOWN_BUTTON)) count++;
    if (arduboy.pressed(LEFT_BUTTON)) count++;
    return count;
}

// Avoid repeating the same target when the water prompt changes.
void advanceWaterTarget() {
    if (waterGame.targetFrames > 0) {
        waterGame.targetFrames--;
        return;
    }

    WaterTargetButton next = randomWaterTarget();
    if (next == waterGame.target) {
        next = static_cast<WaterTargetButton>((next + 1) % 4);
    }
    waterGame.target = next;
    waterGame.targetFrames = randomWaterTargetDuration();
}

void updateWaterMinigame() {
    advanceWaterTarget();

    if (waterGame.progress > 0 && arduboy.everyXFrames(WATER_DRAIN_INTERVAL)) {
        waterGame.progress--;
    }
    if (waterGame.sprayFrames > 0) {
        waterGame.sprayFrames--;
    }

    if (pressedWaterDirectionCount() != 1) {
        return;
    }

    // Only fresh presses count, so holding a button cannot fill the bar.
    uint8_t targetMask = waterButtonMask(waterGame.target);
    if (!arduboy.justPressed(targetMask)) {
        return;
    }

    uint8_t nextProgress = waterGame.progress + WATER_PROGRESS_STEP;
    waterGame.progress = nextProgress > WATER_PROGRESS_MAX ? WATER_PROGRESS_MAX : nextProgress;
    waterGame.sprayFrames = WATER_SPRAY_FEEDBACK_FRAMES;
    sound.tones(feedCatch);
}

// Scratches alternate sides to make the idle and water scenes feel active.
void drawScratchMarks() {
    if ((frameCounter / framesAtGameFps(20)) % 2 == 0) {
        arduboy.drawFastVLine(8, 14, 7, BLACK);
        arduboy.drawFastVLine(10, 12, 10, BLACK);
        arduboy.drawFastVLine(12, 13, 8, BLACK);
        arduboy.drawFastVLine(14, 14, 7, BLACK);
    } else {
        arduboy.drawFastVLine(60, 8, 7, BLACK);
        arduboy.drawFastVLine(62, 6, 10, BLACK);
        arduboy.drawFastVLine(64, 7, 8, BLACK);
        arduboy.drawFastVLine(66, 8, 7, BLACK);
    }
}

// Reuse the main-screen eye overlay so the water cat matches idle sleep art.
void drawWaterClosedEyes() {
    closeEyes();
}

void drawSprayBottle(bool pressed) {
    Sprites::drawOverwrite(WATER_SPRAY_X, WATER_SPRAY_Y, waterSpray, pressed ? 0 : 1);
}

void drawSprayParticles() {
    uint8_t nozzleX = WATER_SPRAY_X + WATER_SPRAY_NOZZLE_X;
    uint8_t nozzleY = WATER_SPRAY_Y + WATER_SPRAY_NOZZLE_Y;
    arduboy.drawFastHLine(nozzleX - 14, nozzleY, 9, BLACK);
    arduboy.drawLine(nozzleX - 13, nozzleY - 5, nozzleX - 6, nozzleY - 2, BLACK);
    arduboy.drawLine(nozzleX - 13, nozzleY + 5, nozzleX - 6, nozzleY + 2, BLACK);
}

// The editor stores only the UP d-pad sprite; the other directions are transforms.
bool transformedDpadPixelIsBlack(WaterTargetButton direction, uint8_t x, uint8_t y) {
    switch (direction)
    {
    case WATER_TARGET_RIGHT:
        return spritePixelIsBlack(waterDpadButton, 0, y, 7 - x);
    case WATER_TARGET_DOWN:
        return spritePixelIsBlack(waterDpadButton, 0, 7 - x, 7 - y);
    case WATER_TARGET_LEFT:
        return spritePixelIsBlack(waterDpadButton, 0, 7 - y, x);
    case WATER_TARGET_UP:
    default:
        return spritePixelIsBlack(waterDpadButton, 0, x, y);
    }
}

// Active prompt fills the outline without needing separate filled sprites.
void drawDpadButton(uint8_t x, uint8_t y, WaterTargetButton direction, bool active) {
    for (uint8_t row = 0; row < 8; row++) {
        int8_t first = -1;
        int8_t last = -1;
        for (uint8_t col = 0; col < 8; col++) {
            if (!transformedDpadPixelIsBlack(direction, col, row)) {
                continue;
            }
            arduboy.drawPixel(x + col, y + row, BLACK);
            if (first < 0) {
                first = col;
            }
            last = col;
        }
        if (active && first >= 0 && last > first) {
            arduboy.drawFastHLine(x + first, y + row, last - first + 1, BLACK);
        }
    }
}

void drawWaterDpad() {
    constexpr uint8_t cx = 100;
    constexpr uint8_t cy = 28;
    drawDpadButton(cx, cy - 5, WATER_TARGET_UP, waterGame.target == WATER_TARGET_UP);
    drawDpadButton(cx + 5, cy, WATER_TARGET_RIGHT, waterGame.target == WATER_TARGET_RIGHT);
    drawDpadButton(cx, cy + 5, WATER_TARGET_DOWN, waterGame.target == WATER_TARGET_DOWN);
    drawDpadButton(cx - 5, cy, WATER_TARGET_LEFT, waterGame.target == WATER_TARGET_LEFT);
}

void drawWaterProgressBar() {
    arduboy.drawRect(WATER_BAR_X, WATER_BAR_Y, WATER_BAR_W, WATER_BAR_H, BLACK);
    uint8_t fill = (static_cast<uint16_t>(waterGame.progress) * (WATER_BAR_W - 2)) / WATER_PROGRESS_MAX;
    if (fill > 0) {
        arduboy.fillRect(WATER_BAR_X + 1, WATER_BAR_Y + 1, fill, WATER_BAR_H - 2, BLACK);
    }
}

void drawWaterMinigame() {
    drawScratchMarks();
    Sprites::drawOverwrite(PET_IDLE_DRAW_X, PET_IDLE_DRAW_Y, PET_IDLE1, 0);
    bool spraying = waterGame.sprayFrames > 0;
    if (spraying) {
        drawWaterClosedEyes();
        drawSprayParticles();
    }
    drawSprayBottle(spraying);
    drawWaterDpad();
    drawWaterProgressBar();

    tinyfont.setCursor(88, 56);
    tinyfont.print("A EXIT");
}

// Centralized scene transition cleanup prevents stale invert/LED/tool state.
void setState(GAME_STATE status) {
    if (status == CLEAN) {
        resetCleanMinigame();
    }
    if (status == FEED) {
        resetFeedMinigame();
    }
    if (status == PLAY) {
        resetPlayMinigame();
    }
    if (status == WALK) {
        resetWalkMinigame();
    }
    if (status == WATER) {
        resetWaterMinigame();
    }
#ifdef POCKET_PIXEL_FXC_LINK
    if (status == AIR_HOCKEY) {
        resetAirHockeyMinigame();
        stopVisitHost();
    }
#endif
    if (state == FEED && status != FEED) {
        arduboy.invert(false);
    }
    if (state == PLAY && status != PLAY) {
        arduboy.invert(false);
    }
    if (state == WALK && status != WALK) {
        arduboy.invert(false);
    }
    if (state == WATER && status != WATER) {
        arduboy.digitalWriteRGB(RGB_OFF, RGB_OFF, RGB_OFF);
    }
#ifdef POCKET_PIXEL_FXC_LINK
    if (state == AIR_HOCKEY && status != AIR_HOCKEY) {
        arduboy.invert(false);
    }
#endif
    state = status;
}

// Common care completion path updates pet state, rewards, save, and returns home.
void finishCareAction(PetStatus status, uint8_t xpReward, uint16_t moneyReward) {
    petUnset(status);
    addXpTenths(xpReward);
    addMoney(moneyReward);
    sound.tones(feedDone);
    saveGame();
    setState(IDLE);
}

void feed() {
    if (arduboy.justPressed(A_BUTTON)) {
        arduboy.invert(false);
        if (feedGame.initialized) {
            // Early exit keeps whatever bowl progress the player earned.
            applyFeedLifeProgress();
        }
        saveGame();
        setState(IDLE);
        return;
    }

    if (pet.life == MAX_LIFE) {
        // Hunger is derived from life, so full life makes feed unavailable.
        setState(IDLE);
        return;
    }

    if (!feedGame.initialized) {
        initFeedMinigame();
    }

    updateFeedMinigame();
    if (feedGame.foodUnits >= FEED_FULL_UNITS) {
        finishFeedMinigame();
        return;
    }

    drawFeedMinigame();
}

// Legacy simple action screen kept for one-button care flows if needed again.
void careScreen(const char *title, const char *action, PetStatus status, uint8_t xpReward, uint16_t moneyReward) {
    if (arduboy.justPressed(A_BUTTON)) {
        setState(IDLE);
        return;
    }

    if (arduboy.justPressed(B_BUTTON)) {
        finishCareAction(status, xpReward, moneyReward);
        return;
    }

    Sprites::drawOverwrite(PET_IDLE_DRAW_X, PET_IDLE_DRAW_Y, PET_IDLE2, 0);
    drawLifeBar(arduboy);
    tinyfont.setCursor(3, 52);
    tinyfont.print(title);
    drawButtonHint(arduboy, tinyfont, 56, "EXIT");
    drawButtonHint(arduboy, tinyfont, 49, action);
}

void play() {
    if (arduboy.justPressed(A_BUTTON)) {
        resetPlayMinigame();
        arduboy.invert(false);
        setState(IDLE);
        return;
    }

    if (!petHas(BORED)) {
        // If the underlying need was cleared elsewhere, leave the minigame.
        resetPlayMinigame();
        arduboy.invert(false);
        setState(IDLE);
        return;
    }

    if (!playGame.initialized) {
        initPlayMinigame();
    }

    updatePlayMinigame();
    if (state != PLAY) {
        return;
    }

    drawPlayMinigame();
}

#ifdef POCKET_PIXEL_FXC_LINK
void airHockey() {
    if (!linkPeerAvailable()) {
        resetAirHockeyMinigame();
        arduboy.invert(false);
        setState(IDLE);
        return;
    }

    if (!hockeyGame.initialized) {
        initAirHockeyMinigame();
    }

    if (arduboy.pressed(A_BUTTON)) {
        if (hockeyGame.exitHoldFrames < HOCKEY_EXIT_HOLD_FRAMES) {
            hockeyGame.exitHoldFrames++;
        }
        if (hockeyGame.exitHoldFrames >= HOCKEY_EXIT_HOLD_FRAMES) {
            resetAirHockeyMinigame();
            arduboy.invert(false);
            setState(IDLE);
            return;
        }
    } else {
        hockeyGame.exitHoldFrames = 0;
    }

    updateAirHockeyMinigame();
    if (hockeyGame.hostScore >= HOCKEY_SCORE_REQUIRED || hockeyGame.clientScore >= HOCKEY_SCORE_REQUIRED) {
        finishAirHockeyMinigame();
        return;
    }

    drawAirHockeyMinigame();
}

void drawVisitMenuOption(uint8_t index, const char *label) {
    tinyfont.setCursor(48, 29 + index * 8);
    tinyfont.print(visitMenuState.selected == index ? ">" : " ");
    tinyfont.print(label);
}

void drawRamSpriteMirrored(int16_t x, int16_t y, const uint8_t *sprite) {
    uint8_t w = sprite[0];
    uint8_t h = sprite[1];
    uint8_t pages = (h + 7) / 8;
    for (uint8_t sx = 0; sx < w; sx++) {
        for (uint8_t page = 0; page < pages; page++) {
            uint8_t bits = sprite[2 + sx + page * w];
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t sy = page * 8 + bit;
                if (sy >= h) {
                    continue;
                }
                if ((bits & (1 << bit)) != 0) {
                    arduboy.drawPixel(x + w - 1 - sx, y + sy, BLACK);
                }
            }
        }
    }
}

void drawVisitHostScene() {
    int16_t hostX = 14;
    int16_t guestX = 82;
    uint8_t localFrame = (frameCounter / idleBreathFrames()) & 1;
    Sprites::drawOverwrite(hostX, PET_IDLE_DRAW_Y, localFrame ? PET_IDLE1 : PET_IDLE2, 0);

    if (remoteVisit.spriteReady[0] && remoteVisit.spriteReady[1]) {
        uint8_t remoteBreath = remoteVisit.profile.breathFrames == 0 ? idleBreathFrames() : remoteVisit.profile.breathFrames;
        uint8_t remoteFrame = (frameCounter / remoteBreath) & 1;
        int16_t guestY = PET_IDLE_DRAW_Y;
        if (visitHostState.frame < VISIT_LEAVE_FRAMES) {
            guestY = 64 - ((64 - PET_IDLE_DRAW_Y) * visitHostState.frame) / VISIT_LEAVE_FRAMES;
            visitHostState.frame++;
        }
        drawRamSpriteMirrored(guestX, guestY, remoteVisit.idleSprites[remoteFrame]);
    } else {
        tinyfont.setCursor(72, 30);
        tinyfont.print("...");
    }
}

void drawVisitMenu() {
    if (visitMenuState.frame < VISIT_LEAVE_FRAMES) {
        uint8_t y = PET_IDLE_DRAW_Y + visitMenuState.frame * 2;
        Sprites::drawOverwrite(PET_IDLE_DRAW_X, y, (visitMenuState.frame / 4) & 1 ? PET_IDLE1 : PET_IDLE2, 0);
        visitMenuState.frame++;
        return;
    }

    if (visitMenuState.frame < VISIT_LEAVE_FRAMES + VISIT_PAUSE_FRAMES) {
        visitMenuState.frame++;
        return;
    }

    uint8_t shutterFrame = visitMenuState.frame - VISIT_LEAVE_FRAMES - VISIT_PAUSE_FRAMES;
    uint8_t h = (uint16_t)64 * shutterFrame / VISIT_SHUTTER_FRAMES;
    if (h < 64) {
        visitMenuState.frame++;
    } else {
        h = 64;
    }
    arduboy.fillRect(0, 0, 128, h, BLACK);
    for (uint8_t y = 3; y < h; y += 6) {
        arduboy.drawFastHLine(0, y, 128, WHITE);
    }

    if (h < 64) {
        return;
    }

    arduboy.fillRect(38, 14, 52, 40, WHITE);
    arduboy.drawRect(38, 14, 52, 40, BLACK);
    tinyfont.setCursor(54, 19);
    tinyfont.print("MENU");
    drawVisitMenuOption(0, "BALL");
    drawVisitMenuOption(1, "WATER");
    drawVisitMenuOption(2, "FOOD");
}

void visitMenu() {
    if (!linkPeerAvailable()) {
        setState(IDLE);
        return;
    }
    if (linkConsumeGameStart()) {
        setState(AIR_HOCKEY);
        return;
    }
    if (visitMenuState.frame >= VISIT_LEAVE_FRAMES + VISIT_PAUSE_FRAMES + VISIT_SHUTTER_FRAMES) {
        if (arduboy.justPressed(UP_BUTTON) && visitMenuState.selected > 0) {
            visitMenuState.selected--;
            playMenuMoveTone();
        }
        if (arduboy.justPressed(DOWN_BUTTON) && visitMenuState.selected < 2) {
            visitMenuState.selected++;
            playMenuMoveTone();
        }
        if (arduboy.justPressed(B_BUTTON)) {
            linkSendGameStart();
            setState(AIR_HOCKEY);
            return;
        }
    }
    drawVisitMenu();
}
#endif

void clean() {
    if (!cleanFlag(CLEAN_INITIALIZED)) {
        initCleanMinigame();
    }

    restoreToolOverlay(arduboy);

    if (arduboy.justPressed(A_BUTTON)) {
        resetCleanMinigame();
        setState(IDLE);
        return;
    }

    bool moved = false;
    if (arduboy.pressed(LEFT_BUTTON)) {
        cleanGame.cursorX = clampToLitterX(-CLEAN_TOOL_STEP);
        moved = true;
    }
    if (arduboy.pressed(RIGHT_BUTTON)) {
        cleanGame.cursorX = clampToLitterX(CLEAN_TOOL_STEP);
        moved = true;
    }
    if (arduboy.justPressed(B_BUTTON)) {
        cleanGame.tool = cleanGame.tool == CLEAN_TOOL_RAKE ? CLEAN_TOOL_SCOOP : CLEAN_TOOL_RAKE;
    }
    if (arduboy.pressed(UP_BUTTON)) {
        cleanGame.cursorY = clampToLitterY(-CLEAN_TOOL_STEP);
        moved = true;
    }
    if (arduboy.pressed(DOWN_BUTTON)) {
        cleanGame.cursorY = clampToLitterY(CLEAN_TOOL_STEP);
        moved = true;
    }

    if (moved) {
        uint8_t brushX = cleanBrushX();
        uint8_t brushY = cleanBrushY();
        if (cleanGame.tool == CLEAN_TOOL_RAKE) {
            // Rake mutates sand/reveal state only; scoop mutates collection state.
            drawRakedSandInBrush(arduboy, brushX, brushY);
            revealPoopsInBrush(brushX, brushY);
            revealCoinInBrush(brushX, brushY);
            drawVisiblePoops(arduboy);
        } else {
            scoopPoopsInBrush(arduboy, brushX, brushY);
            collectCoinInBrush(arduboy, brushX, brushY);
        }
        if (cleanComplete()) {
            finishCleanMinigame();
            return;
        }
    }
    if (cleanGame.feedbackFrames > 0) {
        cleanGame.feedbackFrames--;
    }

    drawCleanMinigame(arduboy, tinyfont);
}

void walk() {
    if (arduboy.justPressed(A_BUTTON)) {
        if (walkGame.collectedMoney) {
            // Walk can award optional coins even if the anxiety task is abandoned.
            saveGame();
        }
        resetWalkMinigame();
        arduboy.invert(false);
        setState(IDLE);
        return;
    }

    if (!walkGame.initialized) {
        initWalkMinigame();
    }

    updateWalkMinigame();
    if (walkGame.score >= WALK_SCORE_REQUIRED) {
        resetWalkMinigame();
        arduboy.invert(false);
        finishCareAction(ANXIOUS, 2, 15);
        return;
    }

    drawWalkMinigame();
}

void water() {
    if (arduboy.justPressed(A_BUTTON)) {
        resetWaterMinigame();
        setState(IDLE);
        return;
    }

    if (!petHas(SCRATCHING)) {
        // The siren/scratching status can clear while the scene is open.
        resetWaterMinigame();
        setState(IDLE);
        return;
    }

    if (!waterGame.initialized) {
        initWaterMinigame();
    }

    updateWaterMinigame();
    if (waterGame.progress >= WATER_PROGRESS_MAX) {
        resetWaterMinigame();
        finishCareAction(SCRATCHING, 2, 0);
        return;
    }

    drawWaterMinigame();
}

/************************** IDLE **************************/

constexpr uint8_t MENU_MOVE_TONE_MS = 38;

uint8_t selectedAction = ACTION_FEED;
uint8_t menuMelodyIndex = 0;
bool idleMenuOpen = false;
int16_t idleMenuX = MENU_CLOSED_X;

#ifdef POCKET_PIXEL_FXC_LINK
bool linkInviteConfirmOpen = false;
bool linkInviteConfirmYes = true;

void prepareLinkedIdle() {
    if (!linkIdleActive()) {
        return;
    }
    petUnset(SLEEPING);
    petUnset(SCRATCHING);
    clearZZZ();
    idleMenuOpen = false;
}

void closeLinkInviteConfirm() {
    linkInviteConfirmOpen = false;
    linkInviteConfirmYes = true;
}

void drawLinkInviteConfirm() {
    arduboy.fillRect(34, 18, 60, 27, WHITE);
    arduboy.drawRect(34, 18, 60, 27, BLACK);
    tinyfont.setCursor(48, 23);
    tinyfont.print("INVITE?");
    tinyfont.setCursor(43, 35);
    tinyfont.print(linkInviteConfirmYes ? ">YES" : " YES");
    tinyfont.setCursor(68, 35);
    tinyfont.print(linkInviteConfirmYes ? " NO" : ">NO");
}

bool handleLinkInviteConfirm() {
    if (!linkInviteConfirmOpen) {
        return false;
    }
    if (arduboy.justPressed(LEFT_BUTTON) || arduboy.justPressed(RIGHT_BUTTON)) {
        linkInviteConfirmYes = !linkInviteConfirmYes;
    }
    if (arduboy.justPressed(A_BUTTON)) {
        closeLinkInviteConfirm();
        return true;
    }
    if (arduboy.justPressed(B_BUTTON)) {
        bool accepted = linkInviteConfirmYes;
        closeLinkInviteConfirm();
        if (accepted) {
            startVisitHost();
            startVisitTransfer();
            linkSendInvite();
        }
        return true;
    }
    return true;
}
#endif

int16_t idleMenuTargetX() {
    return idleMenuOpen ? MENU_OPEN_X : MENU_CLOSED_X;
}

// Sliding menu animation is the source for all responsive idle layout.
void updateIdleMenuAnimation() {
    constexpr uint8_t MENU_ANIMATION_STEP = 5;
    int16_t targetX = idleMenuTargetX();
    if (idleMenuX < targetX) {
        idleMenuX += MENU_ANIMATION_STEP;
        if (idleMenuX > targetX) {
            idleMenuX = targetX;
        }
    } else if (idleMenuX > targetX) {
        idleMenuX -= MENU_ANIMATION_STEP;
        if (idleMenuX < targetX) {
            idleMenuX = targetX;
        }
    }
}

// Pet recenters inside whatever width remains before the menu panel.
int16_t idlePetDrawX() {
    int16_t usableWidth = idleMenuX;
    if (usableWidth < PET_IDLE_WIDTH) {
        return PET_IDLE_DRAW_X;
    }

    return (usableWidth - PET_IDLE_WIDTH) / 2;
}

// XP bar stretches with the closed menu but clamps to readable bounds.
uint8_t idleXpWidth() {
    int16_t width = idleMenuX - 4;
    if (width < 30) {
        return 30;
    }
    if (width > 122) {
        return 122;
    }

    return width;
}

#ifdef POCKET_PIXEL_FXC_LINK
void drawLinkSurpriseMark(int16_t petX) {
    if (((frameCounter / framesAtGameFps(5)) & 1) != 0) {
        return;
    }
    uint8_t x = petX + 15;
    arduboy.drawFastVLine(x, 13, 4, BLACK);
    arduboy.drawPixel(x, 19, BLACK);
}
#endif

// Menu movement walks through a profile-defined melody one note at a time.
void playMenuMoveTone() {
    uint16_t note = pgm_read_word(&ActivePersonality.menuMelody[menuMelodyIndex]);
    sound.tone(note, MENU_MOVE_TONE_MS);
    menuMelodyIndex++;
    if (menuMelodyIndex >= ActivePersonality.menuMelodyLength) {
        menuMelodyIndex = 0;
    }
}

// Item selection gates each minigame by its corresponding pet need.
void selectOption() {
    if (!petHas(SLEEPING)) {
        switch (selectedAction)
        {
        case ACTION_FEED:
            if (petHas(HUNGRY)) setState(FEED);
            else sound.tones(denied);
            break;
        case ACTION_PLAY:
            if (petHas(BORED)) setState(PLAY);
            else sound.tones(denied);
            break;
        case ACTION_CLEAN:
            if (petHas(DIRTY)) setState(CLEAN);
            else sound.tones(denied);
            break;
        case ACTION_WALK:
            if (petHas(ANXIOUS)) setState(WALK);
            else sound.tones(denied);
            break;
        case ACTION_WATER:
            if (petHas(SCRATCHING)) setState(WATER);
            else sound.tones(denied);
            break;
        default:
            sound.tones(denied);
            break;
        }
    }
    else sound.tones(denied);
}

// The closed menu still leaves a visible tab; left opens and right closes.
void menu() {
    if (arduboy.justPressed(LEFT_BUTTON)) {
        idleMenuOpen = true;
    }
    if (arduboy.justPressed(RIGHT_BUTTON)) {
        idleMenuOpen = false;
    }

    updateIdleMenuAnimation();

    if (idleMenuOpen && arduboy.justPressed(DOWN_BUTTON)) {
        if (selectedAction + 1 < MENU_ACTION_COUNT) {
            selectedAction++;
            playMenuMoveTone();
        }
    }
    if (idleMenuOpen && arduboy.justPressed(UP_BUTTON)) {
        if (selectedAction > 0) {
            selectedAction--;
            playMenuMoveTone();
        }
    }

    drawMenu(arduboy, idleMenuX, selectedAction);
}

void scratching() {
    static uint8_t scratchFrame = 0;
    if (scratchFrame == 0) {
        takeMoney(100);
    }
    if (scratchFrame < framesAtGameFps(6)) {
        tinyfont.setCursor(32, 10);
        tinyfont.print("-100");
    }

    scratchFrame++;
    if (scratchFrame >= framesAtGameFps(12)) {
        scratchFrame = 0;
    }
    drawScratchMarks();
}

// FX RGB LED siren mirrors the scratching status on real hardware.
void updateScratchSirenLed() {
    if (!petHas(SCRATCHING) || petHas(SLEEPING)) {
        arduboy.digitalWriteRGB(RGB_OFF, RGB_OFF, RGB_OFF);
        return;
    }

    if (((frameCounter / framesAtGameFps(3)) & 1) == 0) {
        arduboy.digitalWriteRGB(RGB_ON, RGB_OFF, RGB_OFF);
    } else {
        arduboy.digitalWriteRGB(RGB_OFF, RGB_OFF, RGB_ON);
    }
}

const uint8_t BD_X = 36;
const uint8_t BD_Y = 15;
const uint8_t BD_WIDTH = 2;
uint8_t boredFrame = 0;
// Bored thought bubble is drawn procedurally to save sprite storage.
void bored(int16_t dx) {
    if (arduboy.everyXFrames(framesAtGameFps(10))) {
        boredFrame++;
        if (boredFrame > 3) boredFrame = 0;
    }
    int16_t x = BD_X + dx;
    arduboy.fillRect(x, BD_Y, BD_WIDTH * 8, BD_WIDTH, WHITE);

    if (boredFrame == 1) {
        arduboy.fillRect(x,      BD_Y, BD_WIDTH, BD_WIDTH, BLACK);
    } else if (boredFrame == 2) {
        arduboy.fillRect(x,      BD_Y, BD_WIDTH, BD_WIDTH, BLACK);
        arduboy.fillRect(x + 5,  BD_Y, BD_WIDTH, BD_WIDTH, BLACK);
    } else if (boredFrame == 3) {
        arduboy.fillRect(x,      BD_Y, BD_WIDTH, BD_WIDTH, BLACK);
        arduboy.fillRect(x + 5,  BD_Y, BD_WIDTH, BD_WIDTH, BLACK);
        arduboy.fillRect(x + 10, BD_Y, BD_WIDTH, BD_WIDTH, BLACK);
    }
}

// Sleep/water eye overlays assume the same pet anchor and accept a draw offset.
void closeEyes(int16_t dx) {
    arduboy.drawPixel(42 + dx, 26, WHITE);
    arduboy.drawPixel(44 + dx, 26, WHITE);
}

// Anxiety draws short stress marks above the idle pet.
void anxious(int16_t dx) {
    arduboy.drawFastVLine(39 + dx, 16, 4, BLACK);
    arduboy.drawFastVLine(42 + dx, 16, 5, BLACK);
    arduboy.drawFastVLine(45 + dx, 16, 4, BLACK);
    closeEyes(dx);
}

// Dirty status uses the original stink animation on the main screen.
void dirty(int16_t dx) {
    if ((frameCounter / framesAtGameFps(10)) % 2 == 0) {
        Sprites::drawOverwrite(10 + dx, 31, poo1, 0);
    } else {
        Sprites::drawOverwrite(10 + dx, 31, poo2, 0);
    }
}

const uint8_t SLEEP_X = 52;
const uint8_t SLEEP_Y = 1;
constexpr uint8_t ZZZ_WIDTH = 21;
constexpr uint8_t ZZZ_HEIGHT = 22;

uint8_t sleepFrame = 0;
void clearZZZ() {
    sleepFrame = 0;
}

// Z glyphs are procedural because the animation is tiny and storage matters.
void drawSmallZ(int16_t x, uint8_t y) {
    arduboy.drawFastHLine(x, y, 4, BLACK);
    arduboy.drawPixel(x + 2, y + 1, BLACK);
    arduboy.drawPixel(x + 1, y + 2, BLACK);
    arduboy.drawFastHLine(x, y + 3, 4, BLACK);
}

void drawMediumZ(int16_t x, uint8_t y) {
    arduboy.drawFastHLine(x, y, 6, BLACK);
    arduboy.drawFastHLine(x + 4, y + 1, 2, BLACK);
    arduboy.drawPixel(x + 3, y + 2, BLACK);
    arduboy.drawPixel(x + 2, y + 3, BLACK);
    arduboy.drawFastHLine(x, y + 4, 2, BLACK);
    arduboy.drawFastHLine(x, y + 5, 6, BLACK);
}

void drawLargeZ(int16_t x, uint8_t y) {
    arduboy.drawFastHLine(x, y, 9, BLACK);
    arduboy.drawFastHLine(x, y + 1, 9, BLACK);
    for (uint8_t row = 0; row < 5; row++) {
        arduboy.drawFastHLine(x + 5 - row, y + 2 + row, 3, BLACK);
    }
    arduboy.drawFastHLine(x, y + 7, 9, BLACK);
    arduboy.drawFastHLine(x, y + 8, 9, BLACK);
}

// Staggered Zs make the sleep status readable without a full sprite.
void drawSleepZzz(uint8_t frame, int16_t dx) {
    int16_t x = SLEEP_X + dx;
    if (frame <= 3) {
        drawSmallZ(x, SLEEP_Y + 18);
    }
    if (frame >= 2 && frame <= 4) {
        drawMediumZ(x + 6, SLEEP_Y + 11);
    }
    if (frame >= 3) {
        drawLargeZ(x + 12, SLEEP_Y);
    }
}

void sleep(int16_t dx) {
    if (arduboy.everyXFrames(framesAtGameFps(15))) sound.tones(snore1);
    if (arduboy.everyXFrames(framesAtGameFps(30))) sound.tones(snore2);

    switch (sleepFrame)
    {
    case 0:
        clearZZZ();
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        drawSleepZzz(sleepFrame, dx);
        break;
    default:
        sleepFrame = 0;
        break;
    }
    arduboy.fillRect(SLEEP_X + dx, ZZZ_HEIGHT + SLEEP_Y, ZZZ_WIDTH, 2, WHITE);
    
    if (arduboy.everyXFrames(framesAtGameFps(7))) {
        sleepFrame = (sleepFrame + 1) % 6;
    }

    closeEyes(dx);
}

// Chances are denominators: chance 50 means roughly one trigger per 50 samples.
uint8_t chance[PET_STATUS_COUNT];

// Copy generated profile probabilities into mutable runtime chance slots.
void initChances() {
    chance[SLEEPING]    = ActivePersonality.chanceSleeping;
    chance[HUNGRY]      = ActivePersonality.chanceHungry;
    chance[DIRTY]       = ActivePersonality.chanceDirty;
    chance[BORED]       = ActivePersonality.chanceBored;
    chance[ANXIOUS]     = ActivePersonality.chanceAnxious;
    chance[SCRATCHING]  = ActivePersonality.chanceScratching;
}

uint8_t bootBias = 10;
// Random status changes are biased down at boot to avoid instant chaos on load.
void randomEmotion() {
    bool sleeping = petHas(SLEEPING);
    for (uint8_t i = 0; i < PET_STATUS_COUNT; i++) {
        if (sleeping && i != SLEEPING) {
            continue;
        }
        uint8_t randSample = random(0,chance[i]);
        if (randSample / bootBias == (chance[i]-1) / bootBias) {
            PetStatus status = static_cast<PetStatus>(i);
            if (petHas(status) && (status == SLEEPING || status == SCRATCHING)) {
                petUnset(status);
                clearZZZ();
            } else if (status == HUNGRY) {
                // Hunger is life loss, not an independently persisted flag.
                hurtPet(1);
            } else {
                if (status == SLEEPING && (petHas(ANXIOUS) || petHas(DIRTY) || petHas(SCRATCHING))) {
                    continue;
                }
                petSet(status);
                if (status == SLEEPING) {
                    sleeping = true;
                }
            }
        }
    }

    bootBias = 1;
}

void idle() {
#ifdef POCKET_PIXEL_FXC_LINK
    if (linkConsumeGameStart()) {
        setState(AIR_HOCKEY);
        return;
    }
    if (linkConsumeInvite()) {
        closeLinkInviteConfirm();
        startVisitTransfer();
        prepareLinkedIdle();
        startVisitMenu();
        return;
    }
    prepareLinkedIdle();
    bool linkedIdle = linkIdleActive();
    if (!linkedIdle) {
        closeLinkInviteConfirm();
        stopVisitHost();
    } else if (linkInviteConfirmOpen) {
        if (handleLinkInviteConfirm() && state != IDLE) {
            return;
        }
    } else if (!visitHostState.active && arduboy.justPressed(A_BUTTON)) {
        linkInviteConfirmOpen = true;
    }
#endif
    if (arduboy.justPressed(B_BUTTON)) {
#ifdef POCKET_PIXEL_FXC_LINK
        if (linkedIdle || linkInviteConfirmOpen) {
            return;
        }
#endif
        if (idleMenuOpen && idleMenuX == MENU_OPEN_X) {
            selectOption();
        } else {
            idleMenuOpen = true;
        }
        return;
    }

    int16_t petX = idlePetDrawX();
    int16_t petDx = petX - PET_IDLE_DRAW_X;

    bool skipIdlePetDraw = false;
#ifdef POCKET_PIXEL_FXC_LINK
    skipIdlePetDraw = linkedIdle && visitHostState.active;
#endif
    // Idle overlays are offset by petDx so menu animation does not desync them.
    if (!skipIdlePetDraw) {
        if ((frameCounter / idleBreathFrames()) % 2 == 0 || petHas(ANXIOUS) || petHas(BORED)) {
            Sprites::drawOverwrite(petX, PET_IDLE_DRAW_Y, PET_IDLE2, 0);
        } else {
            Sprites::drawOverwrite(petX, PET_IDLE_DRAW_Y, PET_IDLE1, 0);
        }
    }

#ifdef POCKET_PIXEL_FXC_LINK
    if (linkedIdle) {
        if (visitHostState.active) {
            drawVisitHostScene();
            drawLifeBar(arduboy);
            drawXpBar(arduboy, tinyfont, 2, idleXpWidth());
        } else {
            menu();
            drawLifeBar(arduboy);
            drawXpBar(arduboy, tinyfont, 2, idleXpWidth());
            tinyfont.setCursor(45, 4);
            tinyfont.print("INVITE?");
            drawLinkSurpriseMark(petX);
        }
        if (linkInviteConfirmOpen) {
            drawLinkInviteConfirm();
        }
        return;
    }
#endif

    if (arduboy.everyXFrames(framesAtGameFps(30))) {
        randomEmotion();
    }

    menu();
    drawLifeBar(arduboy);
    drawXpBar(arduboy, tinyfont, 2, idleXpWidth());
    if (!idleMenuOpen && idleMenuX == MENU_CLOSED_X) {
        // Money HUD is hidden immediately once the menu begins opening.
        drawMoneyHud(tinyfont);
    }
    bool sleeping = petHas(SLEEPING);
    if (sleeping) sleep(petDx);
    else {
        if (petHas(ANXIOUS))     anxious(petDx);
        if (petHas(BORED))       bored(petDx);
        if (petHas(SCRATCHING))  scratching();
    }
    if (!sleeping && petHas(DIRTY)) dirty(petDx);
}

/************************** MAIN **************************/
void gameSetup() {
    // beginDoFirst avoids the stock boot logo so our custom boot can run.
    arduboy.beginDoFirst();
    arduboy.waitNoButtons();
    arduboy.digitalWriteRGB(RGB_OFF, RGB_OFF, RGB_OFF);
    arduboy.setFrameRate(GAME_FPS);
    arduboy.clear();
    tinyfont.setTextColor(BLACK);
    arduboy.fillScreen(WHITE);
    arduboy.initRandomSeed();
#ifdef POCKET_PIXEL_FXC_LINK
    linkBegin(random(1, 65535));
#endif
    playBootAnimation();
    // Load after boot animation so EEPROM delay never happens on a blank screen.
    loadGame();
    setState(IDLE);
    initChances();
    randomEmotion();
}

void gameLoop() {
    if (!(arduboy.nextFrame()))
        return;
    arduboy.pollButtons();
    frameCounter++;
#ifdef POCKET_PIXEL_FXC_LINK
    linkUpdate(state == IDLE || state == VISIT_MENU || state == AIR_HOCKEY);
    updateVisitTransfer();
#endif
    if (state != CLEAN) {
        // Clean owns the framebuffer for its litter canvas and redraws incrementally.
        arduboy.fillScreen(WHITE);
    }

    if (debugHandleFrame(arduboy, tinyfont)) {
        updateScratchSirenLed();
        arduboy.display();
        return;
    }

    switch (state)
    {
    case IDLE:
        idle();
        break;
    case FEED:
        feed();
        break;
    case PLAY:
        play();
        break;
    case CLEAN:
        clean();
        break;
    case WALK:
        walk();
        break;
    case WATER:
        water();
        break;
#ifdef POCKET_PIXEL_FXC_LINK
    case VISIT_MENU:
        visitMenu();
        break;
    case AIR_HOCKEY:
        airHockey();
        break;
#endif
    
    default:
        break;
    }

    updateScratchSirenLed();
    arduboy.display();
}
