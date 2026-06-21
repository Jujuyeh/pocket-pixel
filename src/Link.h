#pragma once

#include <Arduino.h>

struct LinkInput {
    uint8_t x = 64;
    bool strike = false;
};

struct LinkState {
    uint8_t hostX = 64;
    uint8_t clientX = 64;
    uint8_t hostScore = 0;
    uint8_t clientScore = 0;
    uint8_t puckX = 64;
    uint8_t puckY = 64;
    uint8_t flags = 0;
};

void linkBegin(uint32_t seed);
void linkUpdate(bool advertise);
bool linkPeerAvailable();
bool linkLocalIsHost();
bool linkConsumeInvite();
void linkSendInvite();
void linkSendInput(uint8_t x, bool strike);
bool linkConsumeInput(LinkInput &input);
void linkSendState(const LinkState &state);
bool linkConsumeState(LinkState &state);
