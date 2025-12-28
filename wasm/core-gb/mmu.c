/**
 * NeoBoy - Game Boy MMU Implementation
 * 
 * Purpose: Memory access routing and I/O register handling
 */

#include "mmu.h"
#include "ppu.h"
#include "cartridge.h"
#include <string.h>

void gb_mmu_init(gb_mmu_t *mmu, gb_ppu_t *ppu, gb_cartridge_t *cart) {
    memset(mmu, 0, sizeof(gb_mmu_t));
    mmu->ppu = ppu;
    mmu->cart = cart;
    mmu->joypad = 0xFF;
}

void gb_mmu_reset(gb_mmu_t *mmu) {
    memset(mmu->wram, 0, sizeof(mmu->wram));
    memset(mmu->hram, 0, sizeof(mmu->hram));
    memset(mmu->io, 0, sizeof(mmu->io));
    mmu->joypad = 0xFF;
}

u8 gb_mmu_read(gb_mmu_t *mmu, u16 addr) {
    /* ROM Banks (via cartridge) */
    if (addr < 0x8000) {
        return gb_cart_read(mmu->cart, addr);
    }
    
    /* VRAM */
    if (addr >= 0x8000 && addr < 0xA000) {
        return gb_ppu_read_vram(mmu->ppu, addr - 0x8000);
    }
    
    /* External RAM (via cartridge) */
    if (addr >= 0xA000 && addr < 0xC000) {
        return gb_cart_read_ram(mmu->cart, addr - 0xA000);
    }
    
    /* Work RAM */
    if (addr >= 0xC000 && addr < 0xE000) {
        return mmu->wram[addr - 0xC000];
    }
    
    /* Echo RAM (mirror of WRAM) */
    if (addr >= 0xE000 && addr < 0xFE00) {
        return mmu->wram[addr - 0xE000];
    }
    
    /* OAM */
    if (addr >= 0xFE00 && addr < 0xFEA0) {
        return gb_ppu_read_oam(mmu->ppu, addr - 0xFE00);
    }
    
    /* Unusable area */
    if (addr >= 0xFEA0 && addr < 0xFF00) {
        return 0xFF;
    }
    
    /* I/O Registers */
    if (addr >= 0xFF00 && addr < 0xFF80) {
        return mmu->io[addr - 0xFF00];
    }
    
    /* High RAM */
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        return mmu->hram[addr - 0xFF80];
    }
    
    /* Interrupt Enable */
    if (addr == 0xFFFF) {
        return mmu->io[0x7F];
    }
    
    return 0xFF;
}

void gb_mmu_write(gb_mmu_t *mmu, u16 addr, u8 value) {
    /* ROM Banks (may trigger MBC commands) */
    if (addr < 0x8000) {
        gb_cart_write(mmu->cart, addr, value);
        return;
    }
    
    /* VRAM */
    if (addr >= 0x8000 && addr < 0xA000) {
        gb_ppu_write_vram(mmu->ppu, addr - 0x8000, value);
        return;
    }
    
    /* External RAM */
    if (addr >= 0xA000 && addr < 0xC000) {
        gb_cart_write_ram(mmu->cart, addr - 0xA000, value);
        return;
    }
    
    /* Work RAM */
    if (addr >= 0xC000 && addr < 0xE000) {
        mmu->wram[addr - 0xC000] = value;
        return;
    }
    
    /* Echo RAM */
    if (addr >= 0xE000 && addr < 0xFE00) {
        mmu->wram[addr - 0xE000] = value;
        return;
    }
    
    /* OAM */
    if (addr >= 0xFE00 && addr < 0xFEA0) {
        gb_ppu_write_oam(mmu->ppu, addr - 0xFE00, value);
        return;
    }
    
    /* I/O Registers */
    if (addr >= 0xFF00 && addr < 0xFF80) {
        mmu->io[addr - 0xFF00] = value;
        return;
    }
    
    /* High RAM */
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        mmu->hram[addr - 0xFF80] = value;
        return;
    }
    
    /* Interrupt Enable */
    if (addr == 0xFFFF) {
        mmu->io[0x7F] = value;
        return;
    }
}

u16 gb_mmu_read16(gb_mmu_t *mmu, u16 addr) {
    u8 lo = gb_mmu_read(mmu, addr);
    u8 hi = gb_mmu_read(mmu, addr + 1);
    return (hi << 8) | lo;
}

void gb_mmu_write16(gb_mmu_t *mmu, u16 addr, u16 value) {
    gb_mmu_write(mmu, addr, value & 0xFF);
    gb_mmu_write(mmu, addr + 1, value >> 8);
}
