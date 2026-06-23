#pragma once

#include <Arduino.h>
#include <ArduboyTones.h>

// Generated from a Pocket Pixel personality profile. Do not edit by hand.

const uint16_t PROGMEM PixelMenuMelody[] = {
  NOTE_E5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_G5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_E5, NOTE_D5, NOTE_C5, NOTE_C5,
};

const uint16_t PROGMEM PixelMeowTone[] = {
  NOTE_A5, 66, NOTE_AS5, 66, NOTE_A5, 66, NOTE_GS5, 66, TONES_END
};

struct PersonalityProfile {
  const char *name;
  const uint16_t *menuMelody;
  const uint16_t *meowTone;
  uint8_t menuMelodyLength;
  uint8_t playfulness;
  uint8_t anxiety;
  uint8_t chanceSleeping;
  uint8_t chanceHungry;
  uint8_t chanceDirty;
  uint8_t chanceBored;
  uint8_t chanceAnxious;
  uint8_t chanceScratching;
  uint8_t chanceMeow;
  uint8_t feedCost;
  uint8_t fishPreference;
  uint8_t chickenPreference;
};

constexpr PersonalityProfile PixelPersonality = {
  "Pixel",
  PixelMenuMelody,
  PixelMeowTone,
  30,
  30,
  15,
  30,
  90,
  120,
  180,
  255,
  255,
  190,
  20,
  50,
  50,
};

constexpr const PersonalityProfile &ActivePersonality = PixelPersonality;
