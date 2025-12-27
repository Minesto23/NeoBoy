/**
 * NeoBoy - Game Boy Advance Cartridge
 * 
 * GBA ROM format and save types
 * - ROM: 0-32MB
 * - Save types: SRAM, Flash, EEPROM
 */

#include "core.h"
#include <string.h>

typedef enum {
    SAVE_NONE,
    SAVE_SRAM,
    SAVE_FLASH64,
    SAVE_FLASH128,
    SAVE_EEPROM
} SaveType;

static SaveType save_type = SAVE_NONE;

void cartridge_init(void) {
    save_type = SAVE_NONE;
}

/**
 * Detect save type from ROM data
 */
static SaveType detect_save_type(const uint8_t* rom, uint32_t size) {
    // TODO: Scan ROM for save type strings
    // "SRAM_V", "FLASH_V", "FLASH512_V", "FLASH1M_V", "EEPROM_V"
    
    return SAVE_NONE;
}

int cartridge_load_rom(const uint8_t* data, uint32_t size) {
    if (data == NULL || size < 0xC0) {
        return -1;
    }
    
    save_type = detect_save_type(data, size);
    
    return 0;
}

// TODO: Implement SRAM/Flash/EEPROM access
// TODO: Implement save data persistence
