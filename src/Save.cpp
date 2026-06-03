#include "Save.h"

#include "Pet.h"

#include <Arduino.h>
#include <EEPROM.h>

namespace {
constexpr uint8_t SAVE_VERSION = 1;
constexpr uint16_t SAVE_ADDRESS = 128;
constexpr uint16_t SAVE_MAGIC = 0x5050;
constexpr uint8_t DERIVED_STATUS_MASK = 1 << HUNGRY;

struct SaveData {
  uint16_t magic;
  uint8_t version;
  PetState pet;
  uint8_t checksum;
};

// Tiny XOR checksum catches empty/corrupt EEPROM without spending much flash.
uint8_t checksum(const SaveData &data) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&data);
    uint8_t sum = 0;
    for (uint8_t i = 0; i < sizeof(SaveData) - 1; i++) {
        sum ^= bytes[i];
    }
    return sum;
}

// Validate all fields that identify this save format before trusting payload bytes.
bool valid(const SaveData &data) {
    return data.magic == SAVE_MAGIC
        && data.version == SAVE_VERSION
        && data.checksum == checksum(data);
}
}

// Load persistent pet data, falling back to a fresh save when validation fails.
void loadGame() {
    SaveData data;
    EEPROM.get(SAVE_ADDRESS, data);
    if (!valid(data)) {
        resetSave();
        return;
    }

    pet = data.pet;
    // HUNGRY is derived from life, so old or corrupted saves must not persist it.
    pet.statusFlags &= ~DERIVED_STATUS_MASK;
    if (pet.life == 0 || pet.life > MAX_LIFE) {
        pet.life = MAX_LIFE;
    }
    if (pet.xpTenths > 9) {
        pet.xpTenths = 0;
    }
}

// Persist only canonical state; derived flags are stripped before checksum.
void saveGame() {
    PetState savedPet = pet;
    savedPet.statusFlags &= ~DERIVED_STATUS_MASK;
    SaveData data = {
        SAVE_MAGIC,
        SAVE_VERSION,
        savedPet,
        0,
    };
    data.checksum = checksum(data);
    EEPROM.put(SAVE_ADDRESS, data);
}

// Reset is intentionally implemented through saveGame() so EEPROM format stays central.
void resetSave() {
    pet = PetState();
    saveGame();
}
