#include "Pet.h"

PetState pet;

// Treat hunger as a derived condition so save data cannot disagree with life.
bool petHas(PetStatus status) {
    if (status == HUNGRY) {
        return pet.life < MAX_LIFE;
    }
    return (pet.statusFlags & (1 << status)) != 0;
}

// Setting HUNGRY nudges life down instead of storing a separate hunger bit.
void petSet(PetStatus status) {
    if (status == HUNGRY) {
        if (pet.life == MAX_LIFE) {
            hurtPet(1);
        }
        return;
    }
    pet.statusFlags |= 1 << status;
}

// HUNGRY is derived from life, so it cannot be explicitly unset here.
void petUnset(PetStatus status) {
    if (status == HUNGRY) {
        return;
    }
    pet.statusFlags &= ~(1 << status);
}

// Debug helper for non-derived status bits.
void petToggle(PetStatus status) {
    pet.statusFlags ^= 1 << status;
}

// Money intentionally wraps on overflow only at uint16_t bounds; rewards are small.
void addMoney(uint16_t amount) {
    pet.money += amount;
}

// Spending clamps to zero to avoid unsigned underflow.
void takeMoney(uint16_t amount) {
    pet.money = amount > pet.money ? 0 : pet.money - amount;
}

// XP is stored in tenths to keep the bar cheap to draw and save.
void addXpTenths(uint8_t amount) {
    pet.xpTenths += amount;
    while (pet.xpTenths >= 10) {
        pet.level++;
        pet.xpTenths -= 10;
    }
}

// Life never reaches zero in normal play; one half-heart is the minimum alive state.
void setPetLife(uint8_t life) {
    if (life < 1) {
        pet.life = 1;
    } else if (life > MAX_LIFE) {
        pet.life = MAX_LIFE;
    } else {
        pet.life = life;
    }
}

// Shared life recovery path used by care minigames and debug tooling.
void healPet(uint8_t amount) {
    uint8_t healed = pet.life + amount;
    setPetLife(healed);
}

// Shared damage path; clamps through setPetLife().
void hurtPet(uint8_t amount) {
    setPetLife(amount >= pet.life ? 1 : pet.life - amount);
}
