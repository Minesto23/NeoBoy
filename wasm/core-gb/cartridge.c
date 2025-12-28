/**
 * NeoBoy - Game Boy Cartridge and MBC Implementation
 */

#include "core.h"
#include "cartridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        printf("[NeoBoy] [ERROR] Invalid cartridge load: cart=%p, data=%p, size=%u\n", cart, data, size);
        return -1;
    }
    
    // Free previous ROM/RAM
    if (cart->rom) {
        printf("[NeoBoy] Freeing old ROM...\n");
        free(cart->rom);
        cart->rom = NULL;
    }
    if (cart->ram) {
        printf("[NeoBoy] Freeing old RAM...\n");
        free(cart->ram);
        cart->ram = NULL;
    }
    
    // Allocate ROM
    printf("[NeoBoy] Allocating %u bytes for ROM...\n", size);
    cart->rom = (u8*)malloc(size);
    if (!cart->rom) {
        printf("[NeoBoy] [ERROR] Failed to allocate ROM buffer!\n");
        return -1;
    }
    
    memcpy(cart->rom, data, size);
    cart->rom_size = size;
    
    // Parse header
    u8 cart_type = data[0x147];
    cart->mbc_type = detect_mbc_type(cart_type);
    printf("[NeoBoy] Cartridge Type: 0x%02X, MBC: %d\n", cart_type, cart->mbc_type);
    
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
        printf("[NeoBoy] Allocating %u bytes for RAM...\n", cart->ram_size);
        cart->ram = (u8*)calloc(cart->ram_size, 1);
        if (!cart->ram) {
            printf("[NeoBoy] [WARNING] Failed to allocate RAM, but continuing...\n");
            cart->ram_size = 0;
        }
    }
    
    // Copy title
    memcpy(cart->title, &data[0x134], 16);
    cart->title[16] = '\0';
    printf("[NeoBoy] Game Title: %s\n", cart->title);
    
    cart->rom_bank = 1;
    cart->rom_bank_9bit = 1;
    cart->ram_bank = 0;
    cart->ram_enable = false;
    
    return 0;
}

u8 gb_cart_read(gb_cartridge_t *cart, u16 addr) {
    if (!cart || !cart->rom) return 0xFF;
    
    if (addr < 0x4000) {
        /* ROM Bank 0 (or banked in MBC1 Mode 1) */
        u32 bank = 0;
        if (cart->mbc_type == MBC1 && cart->banking_mode == 1) {
            bank = cart->rom_bank & 0x60;
        }
        
        u32 offset = (bank * 0x4000) + addr;
        if (offset < cart->rom_size) {
            return cart->rom[offset];
        } else if (cart->rom_size > 0) {
            return cart->rom[offset % cart->rom_size];
        }
    } else if (addr < 0x8000) {
        /* ROM Bank 01-NN */
        u32 bank = cart->rom_bank;
        if (cart->mbc_type == MBC5) bank = cart->rom_bank_9bit;
        
        /* MBC1/3/5 Bank 0 logic (bank 0 is interpreted as bank 1) */
        if (cart->mbc_type != MBC5 && (bank & 0x1F) == 0) {
            bank |= 1;
        }
        
        u32 offset = (bank * 0x4000) + (addr - 0x4000);
        if (offset < cart->rom_size) {
            return cart->rom[offset];
        } else if (cart->rom_size > 0) {
            // Mirroring
            u32 mirrored_offset = offset % cart->rom_size;
            return cart->rom[mirrored_offset];
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
                /* ROM Bank Select (lower 5 bits) */
                u8 bank = value & 0x1F;
                if (bank == 0) bank = 1;
                cart->rom_bank = (cart->rom_bank & 0x60) | bank;
            } else if (addr < 0x6000) {
                /* RAM Bank Select / Upper ROM Bank Bits */
                if (cart->banking_mode == 0) {
                    /* ROM Banking Mode: ROM bank bits 5-6 */
                    cart->rom_bank = (cart->rom_bank & 0x1F) | ((value & 0x03) << 5);
                } else {
                    /* RAM Banking Mode: RAM bank select */
                    cart->ram_bank = value & 0x03;
                }
            } else if (addr < 0x8000) {
                /* Banking Mode Select */
                cart->banking_mode = value & 0x01;
            }
            break;
            
        case MBC3:
            if (addr < 0x2000) {
                cart->ram_enable = (value & 0x0F) == 0x0A;
            } else if (addr < 0x4000) {
                /* ROM Bank Select (7 bits) */
                u8 bank = value & 0x7F;
                if (bank == 0) bank = 1;
                cart->rom_bank = bank;
            } else if (addr < 0x6000) {
                /* RAM Bank Select / RTC Register Select */
                cart->ram_bank = value;
            } else if (addr < 0x8000) {
                /* Latch Clock Data */
                if (value == 0x01 && !cart->rtc_latched) {
                    /* Latch RTC (simplified: just copy current regs to latch) */
                    memcpy(cart->rtc_latch, cart->rtc_regs, 5);
                }
                cart->rtc_latched = (value == 0x01);
            }
            break;

        case MBC5:
            if (addr < 0x2000) {
                cart->ram_enable = (value & 0x0F) == 0x0A;
            } else if (addr < 0x3000) {
                /* ROM Bank Select (lower 8 bits) */
                cart->rom_bank_9bit = (cart->rom_bank_9bit & 0x100) | value;
            } else if (addr < 0x4000) {
                /* ROM Bank Select (9th bit) */
                cart->rom_bank_9bit = (cart->rom_bank_9bit & 0xFF) | ((value & 0x01) << 8);
            } else if (addr < 0x6000) {
                /* RAM Bank Select */
                cart->ram_bank = value & 0x0F;
            }
            break;
            
        default:
            break;
    }
}

u8 gb_cart_read_ram(gb_cartridge_t *cart, u16 addr) {
    if (!cart || !cart->ram_enable) return 0xFF;
    
    if (cart->mbc_type == MBC3 && cart->ram_bank >= 0x08 && cart->ram_bank <= 0x0C) {
        /* Read RTC Register */
        return cart->rtc_latched ? cart->rtc_latch[cart->ram_bank - 0x08] : cart->rtc_regs[cart->ram_bank - 0x08];
    }

    if (!cart->ram) return 0xFF;

    u32 offset = (cart->ram_bank * 0x2000) + (addr - 0xA000);
    if (offset < cart->ram_size) {
        return cart->ram[offset];
    }
    
    return 0xFF;
}

void gb_cart_write_ram(gb_cartridge_t *cart, u16 addr, u8 value) {
    if (!cart || !cart->ram_enable) return;

    if (cart->mbc_type == MBC3 && cart->ram_bank >= 0x08 && cart->ram_bank <= 0x0C) {
        /* Write RTC Register */
        cart->rtc_regs[cart->ram_bank - 0x08] = value;
        return;
    }
    
    if (!cart->ram) return;

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
