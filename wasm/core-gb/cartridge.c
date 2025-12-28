/**
 * NeoBoy - Game Boy Cartridge and MBC Implementation
 */

#include "core.h"
#include "cartridge.h"
#include <stdlib.h>
#include <string.h>

/**
 * Detect MBC type from cartridge header
 */
static mbc_type_t detect_mbc_type(u8 cart_type) {
    switch (cart_type) {
        case 0x00: return MBC_NONE;
        case 0x01:
        case 0x02:
        case 0x03: return MBC1;
        case 0x05:
        case 0x06: return MBC2;
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: return MBC3;
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E: return MBC5;
        default: return MBC_NONE;
    }
}

void gb_cart_init(gb_cartridge_t *cart) {
    if (!cart) return;
    memset(cart, 0, sizeof(gb_cartridge_t));
    cart->rom_bank = 1;
}

int gb_cart_load(gb_cartridge_t *cart, const u8 *data, u32 size) {
    if (!cart || !data || size < 0x150) {
        return -1;
    }
    
    // Free previous ROM/RAM
    if (cart->rom) free(cart->rom);
    if (cart->ram) free(cart->ram);
    
    // Allocate ROM
    cart->rom = (u8*)malloc(size);
    if (!cart->rom) return -1;
    
    memcpy(cart->rom, data, size);
    cart->rom_size = size;
    
    // Parse header
    u8 cart_type = data[0x147];
    cart->mbc_type = detect_mbc_type(cart_type);
    
    // Parse RAM size
    u8 ram_size_code = data[0x149];
    switch (ram_size_code) {
        case 0x01: cart->ram_size = 2 * 1024; break;
        case 0x02: cart->ram_size = 8 * 1024; break;
        case 0x03: cart->ram_size = 32 * 1024; break;
        case 0x04: cart->ram_size = 128 * 1024; break;
        case 0x05: cart->ram_size = 64 * 1024; break;
        default: cart->ram_size = 0; break;
    }
    
    if (cart->ram_size > 0) {
        cart->ram = (u8*)calloc(cart->ram_size, 1);
    }
    
    // Copy title
    memcpy(cart->title, &data[0x134], 16);
    cart->title[16] = '\0';
    
    cart->rom_bank = 1;
    cart->ram_bank = 0;
    cart->ram_enable = false;
    
    return 0;
}

u8 gb_cart_read(gb_cartridge_t *cart, u16 addr) {
    if (!cart || !cart->rom) return 0xFF;
    
    if (addr < 0x4000) {
        return cart->rom[addr];
    } else if (addr < 0x8000) {
        u32 offset = (cart->rom_bank * 0x4000) + (addr - 0x4000);
        if (offset < cart->rom_size) {
            return cart->rom[offset];
        }
    }
    
    return 0xFF;
}

void gb_cart_write(gb_cartridge_t *cart, u16 addr, u8 value) {
    if (!cart) return;
    
    switch (cart->mbc_type) {
        case MBC1:
            if (addr < 0x2000) {
                cart->ram_enable = (value & 0x0F) == 0x0A;
            } else if (addr < 0x4000) {
                u8 bank = value & 0x1F;
                if (bank == 0) bank = 1;
                cart->rom_bank = (cart->rom_bank & 0x60) | bank;
            } else if (addr < 0x6000) {
                cart->ram_bank = value & 0x03;
            } else if (addr < 0x8000) {
                cart->banking_mode = value & 0x01;
            }
            break;
            
        case MBC3:
            if (addr < 0x2000) {
                cart->ram_enable = (value & 0x0F) == 0x0A;
            } else if (addr < 0x4000) {
                u8 bank = value & 0x7F;
                if (bank == 0) bank = 1;
                cart->rom_bank = bank;
            } else if (addr < 0x6000) {
                cart->ram_bank = value & 0x03;
            }
            break;
            
        default:
            break;
    }
}

u8 gb_cart_read_ram(gb_cartridge_t *cart, u16 addr) {
    if (!cart || !cart->ram || !cart->ram_enable) return 0xFF;
    
    u32 offset = (cart->ram_bank * 0x2000) + (addr - 0xA000);
    if (offset < cart->ram_size) {
        return cart->ram[offset];
    }
    
    return 0xFF;
}

void gb_cart_write_ram(gb_cartridge_t *cart, u16 addr, u8 value) {
    if (!cart || !cart->ram || !cart->ram_enable) return;
    
    u32 offset = (cart->ram_bank * 0x2000) + (addr - 0xA000);
    if (offset < cart->ram_size) {
        cart->ram[offset] = value;
    }
}

void gb_cart_destroy(gb_cartridge_t *cart) {
    if (!cart) return;
    if (cart->rom) free(cart->rom);
    if (cart->ram) free(cart->ram);
    cart->rom = NULL;
    cart->ram = NULL;
}
