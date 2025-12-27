/**
 * NeoBoy - Game Boy Cartridge and MBC Implementation
 * 
 * Handles ROM loading and Memory Bank Controllers (MBC)
 * 
 * Supported MBCs:
 * - MBC1: Up to 2MB ROM, 32KB RAM
 * - MBC2: Up to 256KB ROM, 512x4 bits RAM
 * - MBC3: Up to 2MB ROM, 32KB RAM, RTC
 * - MBC5: Up to 8MB ROM, 128KB RAM
 */

#include "core.h"
#include <stdlib.h>
#include <string.h>

typedef enum {
    MBC_NONE = 0,
    MBC_MBC1,
    MBC_MBC2,
    MBC_MBC3,
    MBC_MBC5
} MBCType;

typedef struct {
    MBCType type;
    uint8_t* rom;
    uint32_t rom_size;
    uint8_t* ram;
    uint32_t ram_size;
    int rom_bank;
    int ram_bank;
    bool ram_enabled;
} Cartridge;

static Cartridge cart;

void cartridge_init(void) {
    memset(&cart, 0, sizeof(Cartridge));
    cart.rom_bank = 1;
}

/**
 * Detect MBC type from cartridge header
 */
static MBCType detect_mbc_type(uint8_t cart_type) {
    switch (cart_type) {
        case 0x00: return MBC_NONE;
        case 0x01:
        case 0x02:
        case 0x03: return MBC_MBC1;
        case 0x05:
        case 0x06: return MBC_MBC2;
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: return MBC_MBC3;
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E: return MBC_MBC5;
        default: return MBC_NONE;
    }
}

int cartridge_load_rom(const uint8_t* data, uint32_t size) {
    if (data == NULL || size < 0x150) {
        return -1;  // Invalid ROM
    }
    
    // Free previous cartridge
    if (cart.rom) {
        free(cart.rom);
    }
    if (cart.ram) {
        free(cart.ram);
    }
    
    // Allocate and copy ROM
    cart.rom = (uint8_t*)malloc(size);
    memcpy(cart.rom, data, size);
    cart.rom_size = size;
    
    // Read cartridge header
    uint8_t cart_type = data[0x147];
    cart.type = detect_mbc_type(cart_type);
    
    // Allocate RAM based on header
    uint8_t ram_size_code = data[0x149];
    switch (ram_size_code) {
        case 0x00: cart.ram_size = 0; break;
        case 0x01: cart.ram_size = 2 * 1024; break;
        case 0x02: cart.ram_size = 8 * 1024; break;
        case 0x03: cart.ram_size = 32 * 1024; break;
        case 0x04: cart.ram_size = 128 * 1024; break;
        case 0x05: cart.ram_size = 64 * 1024; break;
        default: cart.ram_size = 0; break;
    }
    
    if (cart.ram_size > 0) {
        cart.ram = (uint8_t*)calloc(cart.ram_size, 1);
    }
    
    return 0;
}

/**
 * Handle MBC write operations
 */
void cartridge_write(uint16_t address, uint8_t value) {
    // TODO: Implement MBC-specific banking logic
    
    switch (cart.type) {
        case MBC_MBC1:
            if (address < 0x2000) {
                // RAM Enable
                cart.ram_enabled = (value & 0x0F) == 0x0A;
            } else if (address < 0x4000) {
                // ROM Bank Number (lower 5 bits)
                cart.rom_bank = (cart.rom_bank & 0x60) | (value & 0x1F);
                if (cart.rom_bank == 0) cart.rom_bank = 1;
            }
            break;
            
        // TODO: Implement other MBC types
        default:
            break;
    }
}

uint8_t cartridge_read_rom(uint16_t address) {
    if (address < 0x4000) {
        // Bank 0
        return cart.rom ? cart.rom[address] : 0xFF;
    } else {
        // Banked ROM
        uint32_t offset = (cart.rom_bank * 0x4000) + (address - 0x4000);
        if (offset < cart.rom_size) {
            return cart.rom[offset];
        }
        return 0xFF;
    }
}

uint8_t cartridge_read_ram(uint16_t address) {
    if (!cart.ram_enabled || !cart.ram) {
        return 0xFF;
    }
    
    uint32_t offset = (cart.ram_bank * 0x2000) + (address - 0xA000);
    if (offset < cart.ram_size) {
        return cart.ram[offset];
    }
    return 0xFF;
}

void cartridge_write_ram(uint16_t address, uint8_t value) {
    if (!cart.ram_enabled || !cart.ram) {
        return;
    }
    
    uint32_t offset = (cart.ram_bank * 0x2000) + (address - 0xA000);
    if (offset < cart.ram_size) {
        cart.ram[offset] = value;
    }
}

void cartridge_destroy(void) {
    if (cart.rom) {
        free(cart.rom);
        cart.rom = NULL;
    }
    if (cart.ram) {
        free(cart.ram);
        cart.ram = NULL;
    }
}

// TODO: Implement save RAM persistence
// TODO: Implement RTC for MBC3
