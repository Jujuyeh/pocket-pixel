#pragma once

#include <Arduino.h>

struct LinkInput {
    uint8_t x;
    bool strike;
};

struct LinkState {
    uint8_t hostX;
    uint8_t clientX;
    uint8_t hostScore;
    uint8_t clientScore;
    uint8_t puckX;
    uint8_t puckY;
    uint8_t flags;
};

constexpr uint8_t LINK_SPRITE_CHUNK_BYTES = 5;

enum LinkGameKind : uint8_t {
    LINK_GAME_BALL_HUNT = 0,
    LINK_GAME_WATER_BATTLE = 1,
    LINK_GAME_FOOD_RUSH = 2,
};

enum LinkVisitSprite : uint8_t {
    LINK_VISIT_SPRITE_IDLE1 = 0,
    LINK_VISIT_SPRITE_IDLE2 = 1,
};

struct LinkVisitProfile {
    uint16_t meowNote;
    uint8_t breathFrames;
    uint8_t playfulness;
    uint8_t fishPreference;
    uint8_t chickenPreference;
};

struct LinkSpriteChunk {
    uint8_t sprite;
    uint8_t chunk;
    uint8_t data[LINK_SPRITE_CHUNK_BYTES];
};

void linkBegin(uint32_t seed);
void linkUpdate(bool advertise);
bool linkPeerAvailable();
bool linkLocalIsHost();
bool linkConsumeInvite();
void linkSendInvite();
bool linkConsumeGameStart(uint8_t &game);
void linkSendGameStart(uint8_t game);
void linkSendInput(uint8_t x, bool strike);
bool linkConsumeInput(LinkInput &input);
void linkSendState(const LinkState &state);
bool linkConsumeState(LinkState &state);
bool linkSendVisitProfile(const LinkVisitProfile &profile);
bool linkConsumeVisitProfile(LinkVisitProfile &profile);
bool linkSendSpriteChunk(const LinkSpriteChunk &chunk);
bool linkConsumeSpriteChunk(LinkSpriteChunk &chunk);
