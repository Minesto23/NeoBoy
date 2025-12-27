/**
 * NeoBoy - Game Boy Color MMU
 * 
 * Extends GB MMU with:
 * - VRAM banking (2 banks, 16KB total)
 * - WRAM banking (7 switchable banks)
 * - CGB-specific I/O registers
 */

#include "core.h"
#include <string.h>

static uint8_t vram[0x4000];  // 16KB (2 banks)
static uint8_t wram[0x8000];  // 32KB (8 banks)
static int vram_bank = 0;
static int wram_bank = 1;

void mmu_init(void) {
    memset(vram, 0, sizeof(vram));
    memset(wram, 0, sizeof(wram));
    vram_bank = 0;
    wram_bank = 1;
}

uint8_t mmu_read(uint16_t address) {
    // VRAM (banked)
    if (address >= 0x8000 && address < 0xA000) {
        return vram[(vram_bank * 0x2000) + (address - 0x8000)];
    }
    // WRAM (banked)
    else if (address >= 0xD000 && address < 0xE000) {
        return wram[(wram_bank * 0x1000) + (address - 0xD000)];
    }
    
    // TODO: Implement remaining memory regions
    return 0xFF;
}

void mmu_write(uint16_t address, uint8_t value) {
    // VRAM banking (VBK register at 0xFF4F)
    if (address == 0xFF4F) {
        vram_bank = value & 0x01;
        return;
    }
    // WRAM banking (SVBK register at 0xFF70)
    else if (address == 0xFF70) {
        wram_bank = value & 0x07;
        if (wram_bank == 0) wram_bank = 1;
        return;
    }
    
    // TODO: Implement write handlers
}

// TODO: Implement HDMA (High-speed DMA)
