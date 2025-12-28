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
    
    /* GBC Reset State */
    mmu->svbk = 0x01;  /* WRAM bank 1 selected by default */
    mmu->key1 = 0x00;
    mmu->speed = false; /* Normal speed */
    mmu->hdma_active = false;
    
    if (mmu->ppu) {
        mmu->ppu->vbk = 0x00; /* VRAM bank 0 selected by default */
    }
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
    
    /* VRAM (Banked) */
    if (addr >= 0x8000 && addr < 0xA000) {
        /* Route to PPU with bank info. For now, since gb_ppu_read_vram expects offset from start of VRAM,
           we need to update it to support banking. But wait, mmu.h struct has array. 
           Let's handle banking here by calculating the correct offset into the flat 16KB array. */
        u16 vram_offset = addr - 0x8000;
        if (mmu->ppu->vbk & 0x01) {
            vram_offset += 0x2000; /* Bank 1 at offset 8KB */
        }
        return mmu->ppu->vram[vram_offset];
    }
    
    /* External RAM (via cartridge) */
    if (addr >= 0xA000 && addr < 0xC000) {
        return gb_cart_read_ram(mmu->cart, addr - 0xA000);
    }
    
    /* Work RAM (Bank 0) */
    if (addr >= 0xC000 && addr < 0xD000) {
        return mmu->wram[addr - 0xC000];
    }
    
    /* Work RAM (Bank 1-7, Switchable) */
    if (addr >= 0xD000 && addr < 0xE000) {
        u8 bank = mmu->svbk & 0x07;
        if (bank == 0) bank = 1; /* Bank 0 is treated as Bank 1 for 0xD000-0xDFFF access */
        u32 offset = (bank * 0x1000) + (addr - 0xD000);
        return mmu->wram[offset];
    }
    
    /* Echo RAM (mirror of WRAM) */
    if (addr >= 0xE000 && addr < 0xFE00) {
        /* Echo varies depending on bank, but usually mirrors C000-DDFF. 
           Let's just map it to WRAM simply for now as standard behavior. */
        if (addr < 0xF000) { /* Mirror of Bank 0 */
            return mmu->wram[addr - 0xE000];
        } else { /* Mirror of Bank N */
            u8 bank = mmu->svbk & 0x07;
            if (bank == 0) bank = 1;
            u32 offset = (bank * 0x1000) + (addr - 0xF000);
            return mmu->wram[offset];
        }
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
        
        /* GBC Registers */
        if (addr == IO_VBK) return mmu->ppu->vbk;
        if (addr == IO_SVBK) return mmu->svbk;
        if (addr == IO_KEY1) return mmu->key1 | (mmu->speed ? 0x80 : 0x00);
        
        /* Palette Routes */
        if (addr == IO_BCPS) return mmu->ppu->bcps;
        if (addr == IO_BCPD) return 0xFF; /* Write only typically, or valid? HW allows read */
        if (addr == IO_OCPS) return mmu->ppu->ocps;
        if (addr == IO_OCPD) return 0xFF;

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
    
    /* VRAM (Banked) */
    if (addr >= 0x8000 && addr < 0xA000) {
        u16 vram_offset = addr - 0x8000;
        if (mmu->ppu->vbk & 0x01) {
            vram_offset += 0x2000;
        }
        mmu->ppu->vram[vram_offset] = value;
        return;
    }
    
    /* External RAM */
    if (addr >= 0xA000 && addr < 0xC000) {
        gb_cart_write_ram(mmu->cart, addr - 0xA000, value);
        return;
    }
    
    /* Work RAM (Bank 0) */
    if (addr >= 0xC000 && addr < 0xD000) {
        mmu->wram[addr - 0xC000] = value;
        return;
    }
    
    /* Work RAM (Switchable Bank 1-7) */
    if (addr >= 0xD000 && addr < 0xE000) {
        u8 bank = mmu->svbk & 0x07;
        if (bank == 0) bank = 1;
        u32 offset = (bank * 0x1000) + (addr - 0xD000);
        mmu->wram[offset] = value;
        return;
    }
    
    /* Echo RAM */
    if (addr >= 0xE000 && addr < 0xFE00) {
        if (addr < 0xF000) {
            mmu->wram[addr - 0xE000] = value;
        } else {
            u8 bank = mmu->svbk & 0x07;
            if (bank == 0) bank = 1;
            u32 offset = (bank * 0x1000) + (addr - 0xF000);
            mmu->wram[offset] = value;
        }
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
        
        /* GBC Registers */
        if (addr == IO_VBK) {
            mmu->ppu->vbk = value & 0x01;
            /* Bit 0 determines bank (0 or 1) */
            return;
        }
        if (addr == IO_SVBK) {
            mmu->svbk = value & 0x07;
            /* Bits 0-2 determine bank (0-7, 0 -> 1) */
            return;
        }
        if (addr == IO_KEY1) {
            mmu->key1 = (mmu->key1 & 0x80) | (value & 0x01);
            return;
        }
        
        /* HDMA Registers */
        if (addr >= IO_HDMA1 && addr <= IO_HDMA5) {
            if (addr == IO_HDMA1) mmu->hdma1 = value;
            if (addr == IO_HDMA2) mmu->hdma2 = value;
            if (addr == IO_HDMA3) mmu->hdma3 = value;
            if (addr == IO_HDMA4) mmu->hdma4 = value;
            if (addr == IO_HDMA5) {
                /* Start HDMA or GDMA */
                /* Mode 0: GDMA (General Purpose DMA) - Instant transfer */
                /* Mode 1: HDMA (H-Blank DMA) - 16 bytes per H-Blank */
                
                u16 source = (mmu->hdma1 << 8) | (mmu->hdma2 & 0xF0);
                u16 dest = ((mmu->hdma3 & 0x1F) << 8) | (mmu->hdma4 & 0xF0);
                dest |= 0x8000; /* Always VRAM */
                
                u8 length = (value & 0x7F) + 1; /* Length in 16-byte units */
                
                if (value & 0x80) {
                    /* HDMA (H-Blank) */
                    mmu->hdma5 = value & 0x7F;
                    mmu->hdma_active = true;
                } else {
                    /* GDMA (General Purpose) */
                    for (int i = 0; i < length * 16; i++) {
                        u8 byte = gb_mmu_read(mmu, source + i);
                        gb_mmu_write(mmu, dest + i, byte);
                    }
                    mmu->hdma5 = 0xFF; /* Transfer completed */
                    mmu->hdma_active = false;
                }
            }
            return;
        }
        
        /* Palette Data handling - Route to PPU helper would be cleaner, but implementing here for now */
        /* Actually lets route to PPU writes for these */
        if (addr == IO_BCPS || addr == IO_BCPD || addr == IO_OCPS || addr == IO_OCPD) {
            gb_ppu_write_reg(mmu->ppu, addr, value);
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

void gb_mmu_execute_hdma(gb_mmu_t *mmu) {
    if (!mmu->hdma_active) return;
    
    /* Perform 16-byte transfer */
    u16 source = (mmu->hdma1 << 8) | (mmu->hdma2 & 0xF0);
    u16 dest = ((mmu->hdma3 & 0x1F) << 8) | (mmu->hdma4 & 0xF0);
    dest |= 0x8000; /* Always VRAM */
    
    for (int i = 0; i < 16; i++) {
        u8 byte = gb_mmu_read(mmu, source + i);
        gb_mmu_write(mmu, dest + i, byte);
    }
    
    /* Update source/dest registers */
    source += 16;
    dest += 16;
    
    mmu->hdma1 = (source >> 8) & 0xFF;
    mmu->hdma2 = source & 0xF0;
    mmu->hdma3 = (dest >> 8) & 0x1F;
    mmu->hdma4 = dest & 0xF0;
    
    /* Decrement length (blocks remaining) */
    mmu->hdma5--;
    
    if (mmu->hdma5 == 0xFF) {
        mmu->hdma_active = false;
        mmu->hdma5 = 0xFF; /* Completed */
    } else {
        mmu->hdma5 &= 0x7F; /* Keep bit 7 clear to indicate ongoing? Or just bits 0-6 */
    }
}
