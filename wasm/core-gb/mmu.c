/**
 * NeoBoy - Game Boy MMU Implementation
 * 
 * Purpose: Memory access routing and I/O register handling
 */

#include "mmu.h"
#include "ppu.h"
#include "apu.h"
#include "cartridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void gb_mmu_init(gb_mmu_t *mmu, gb_ppu_t *ppu, gb_apu_t *apu, gb_cartridge_t *cart) {
    memset(mmu, 0, sizeof(gb_mmu_t));
    mmu->ppu = ppu;
    mmu->apu = apu;
    mmu->cart = cart;
    mmu->joypad = 0xFF;
}

void gb_mmu_reset(gb_mmu_t *mmu) {
    memset(mmu->wram, 0, sizeof(mmu->wram));
    memset(mmu->hram, 0, sizeof(mmu->hram));
    memset(mmu->io, 0, sizeof(mmu->io));
    mmu->joypad = 0xFF;
}

static void request_interrupt(gb_mmu_t *mmu, u8 interrupt) {
    u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
    gb_mmu_write(mmu, 0xFF0F, if_reg | interrupt);
}

void gb_mmu_step_timers(gb_mmu_t *mmu, u32 cycles) {
    /* DIV is always incremented at 16384Hz (every 256 cycles) */
    u16 old_div = mmu->div_counter;
    mmu->div_counter += cycles;
    mmu->io[IO_DIV - 0xFF00] = (u8)(mmu->div_counter >> 8);

    /* TIMA (Timer Counter) */
    u8 tac = mmu->io[IO_TAC - 0xFF00];
    if (tac & 0x04) { /* Timer enabled */
        u32 threshold = 0;
        switch (tac & 0x03) {
            case 0: threshold = 1024; break; /* 4096 Hz */
            case 1: threshold = 16;   break; /* 262144 Hz */
            case 2: threshold = 64;   break; /* 65536 Hz */
            case 3: threshold = 256;  break; /* 16384 Hz */
        }

        mmu->tima_counter += cycles;
        while (mmu->tima_counter >= threshold) {
            mmu->tima_counter -= threshold;
            if (mmu->io[IO_TIMA - 0xFF00] == 0xFF) {
                /* Overflow: set TIMA to TMA and request interrupt */
                mmu->io[IO_TIMA - 0xFF00] = mmu->io[IO_TMA - 0xFF00];
                request_interrupt(mmu, 0x04); /* Timer interrupt */
            } else {
                mmu->io[IO_TIMA - 0xFF00]++;
            }
        }
    }
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
        if (addr == 0xFF00) {
            u8 val = mmu->io[0x00] & 0xF0;
            if (!(val & 0x10)) { /* Direction buttons */
                val |= (mmu->joypad >> 4) & 0x0F;
            }
            if (!(val & 0x20)) { /* Action buttons */
                val |= (mmu->joypad & 0x0F);
            }
            return val;
        }

        if (addr >= 0xFF10 && addr < 0xFF40) {
            return gb_apu_read(mmu->apu, addr);
        }
        if (addr >= 0xFF40 && addr <= 0xFF4B) {
            return gb_ppu_read_reg(mmu->ppu, addr);
        }
        return mmu->io[addr - 0xFF00];
    }
    
    /* High RAM */
    if (addr >= 0xFF80 && addr < 0xFFFF) {
        return mmu->hram[addr - 0xFF80];
    }
    
    /* Interrupt Enable */
    if (addr == 0xFFFF) {
        return mmu->ie;
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
        if (addr == IO_DIV) {
            mmu->div_counter = 0;
            mmu->io[0x04] = 0;
            return;
        }
        
        if (addr == IO_SC) {
            if (value == 0x81) {
                printf("%c", mmu->io[IO_SB - 0xFF00]);
                fflush(stdout);
                mmu->io[IO_SC - 0xFF00] = 0x01; /* Reset transfer bit */
                return;
            }
        }
        
        /* Route APU registers (0xFF10-0xFF3F) */
        if (addr >= 0xFF10 && addr < 0xFF40) {
            gb_apu_write(mmu->apu, addr, value);
            return;
        }

        /* Route PPU registers (0xFF40-0xFF4B) */
        if (addr >= 0xFF40 && addr <= 0xFF4B) {
            if (addr == 0xFF46) { /* DMA */
                u16 source = value << 8;
                for (int i = 0; i < 0xA0; i++) {
                    gb_mmu_write(mmu, 0xFE00 + i, gb_mmu_read(mmu, source + i));
                }
            } else {
                gb_ppu_write_reg(mmu->ppu, addr, value);
            }
            return;
        }

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
        mmu->ie = value;
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
