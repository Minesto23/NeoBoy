/**
 * NeoBoy - Game Boy Color Cartridge
 * 
 * Same MBC support as GB, with CGB mode detection
 */

#include "core.h"
#include <string.h>

void cartridge_init(void) {
    // TODO: Initialize cartridge
}

int cartridge_load_rom(const uint8_t* data, uint32_t size) {
    // TODO: Load ROM and detect CGB compatibility
    // Check byte 0x143 for CGB flag
    return 0;
}

// TODO: Detect CGB-only vs CGB-compatible vs DMG-only games
