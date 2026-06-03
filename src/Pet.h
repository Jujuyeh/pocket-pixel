#pragma once

#include <Arduino.h>

enum PetStatus : uint8_t {
  SLEEPING = 0,
  HUNGRY = 1,
  DIRTY = 2,
  BORED = 3,
  ANXIOUS = 4,
  SCRATCHING = 5,
  PET_STATUS_COUNT = 6,
};

constexpr uint8_t MAX_LIFE = 6;

struct PetState {
  uint8_t statusFlags = 0;
  uint8_t life = MAX_LIFE;
  uint8_t level = 0;
  uint8_t xpTenths = 1;
  uint16_t money = 100;
};

extern PetState pet;

bool petHas(PetStatus status);
void petSet(PetStatus status);
void petUnset(PetStatus status);
void petToggle(PetStatus status);

void addMoney(uint16_t amount);
void takeMoney(uint16_t amount);
void addXpTenths(uint8_t amount);
void setPetLife(uint8_t life);
void healPet(uint8_t amount);
void hurtPet(uint8_t amount);
