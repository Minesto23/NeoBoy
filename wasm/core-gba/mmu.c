/**
 * NeoBoy - Game Boy Advance MMU
 * 
 * GBA Memory Map:
 * 0x00000000-0x00003FFF: BIOS (16KB)
 * 0x02000000-0x0203FFFF: EWRAM (256KB)
 * 0x03000000-0x03007FFF: IWRAM (32KB)
 * 0x04000000-0x040003FF: I/O Registers
 * 0x05000000-0x050003FF: Palette RAM (1KB)
 * 0x06000000-0x06017FFF: VRAM (96KB)
 * 0x07000000-0x070003FF: OAM (1KB)
 * 0x08000000-0x09FFFFFF: ROM (Wait State 0, 32MB)
 * 0x0A000000-0x0BFFFFFF: ROM (Wait State 1, 32MB)
 * 0x0C000000-0x0DFFFFFF: ROM (Wait State 2, 32MB)
 * 0x0E000000-0x0E00FFFF: SRAM (64KB max)
 */

#include "core.h"
#include <stdlib.h>
#include <string.h>

// Memory regions
static uint8_t bios[0x4000];           // 16KB BIOS
static uint8_t ewram[0x40000];         // 256KB external work RAM
static uint8_t iwram[0x8000];          // 32KB internal work RAM
static uint8_t io_registers[0x400];    // I/O registers
static uint8_t palette_ram[0x400];     // 1KB palette RAM
static uint8_t vram[0x18000];          // 96KB video RAM
static uint8_t oam[0x400];             // 1KB object attribute memory
static uint8_t* rom = NULL;
static uint32_t rom_size = 0;
static uint8_t sram[0x10000];          // 64KB SRAM

void mmu_init(void) {
    memset(bios, 0, sizeof(bios));
    memset(ewram, 0, sizeof(ewram));
    memset(iwram, 0, sizeof(iwram));
    memset(io_registers, 0, sizeof(io_registers));
    memset(palette_ram, 0, sizeof(palette_ram));
    memset(vram, 0, sizeof(vram));
    memset(oam, 0, sizeof(oam));
    memset(sram, 0, sizeof(sram));
}

int mmu_load_rom(const uint8_t* data, uint32_t size) {
    if (data == NULL || size < 0xC0) {
        return -1;  // Invalid ROM
    }
    
    if (rom) {
        free(rom);
    }
    
    rom = (uint8_t*)malloc(size);
    memcpy(rom, data, size);
    rom_size = size;
    
    return 0;
}

uint32_t mmu_read32(uint32_t address) {
    // Combine 4 bytes into 32-bit value (little-endian)
    uint32_t value = 0;
    value |= mmu_read8(address + 0) << 0;
    value |= mmu_read8(address + 1) << 8;
    value |= mmu_read8(address + 2) << 16;
    value |= mmu_read8(address + 3) << 24;
    return value;
}

uint16_t mmu_read16(uint32_t address) {
    uint16_t value = 0;
    value |= mmu_read8(address + 0) << 0;
    value |= mmu_read8(address + 1) << 8;
    return value;
}

uint8_t mmu_read8(uint32_t address) {
    // BIOS
    if (address < 0x4000) {
        return bios[address];
    }
    // EWRAM
    else if (address >= 0x02000000 && address < 0x02040000) {
        return ewram[address - 0x02000000];
    }
    // IWRAM
    else if (address >= 0x03000000 && address < 0x03008000) {
        return iwram[address - 0x03000000];
    }
    // I/O Registers
    else if (address >= 0x04000000 && address < 0x04000400) {
        return io_registers[address - 0x04000000];
    }
    // Palette RAM
    else if (address >= 0x05000000 && address < 0x05000400) {
        return palette_ram[address - 0x05000000];
    }
    // VRAM
    else if (address >= 0x06000000 && address < 0x06018000) {
        return vram[address - 0x06000000];
    }
    // OAM
    else if (address >= 0x07000000 && address < 0x07000400) {
        return oam[address - 0x07000000];
    }
    // ROM
    else if (address >= 0x08000000 && address < 0x0E000000) {
        uint32_t offset = address - 0x08000000;
        if (rom && offset < rom_size) {
            return rom[offset];
        }
    }
    // SRAM
    else if (address >= 0x0E000000 && address < 0x0E010000) {
        return sram[address - 0x0E000000];
    }
    
    return 0;
}

void mmu_write32(uint32_t address, uint32_t value) {
    mmu_write8(address + 0, (value >> 0) & 0xFF);
    mmu_write8(address + 1, (value >> 8) & 0xFF);
    mmu_write8(address + 2, (value >> 16) & 0xFF);
    mmu_write8(address + 3, (value >> 24) & 0xFF);
}

void mmu_write16(uint32_t address, uint16_t value) {
    mmu_write8(address + 0, (value >> 0) & 0xFF);
    mmu_write8(address + 1, (value >> 8) & 0xFF);
}

void mmu_write8(uint32_t address, uint8_t value) {
    // BIOS (read-only)
    if (address < 0x4000) {
        return;
    }
    // EWRAM
    else if (address >= 0x02000000 && address < 0x02040000) {
        ewram[address - 0x02000000] = value;
    }
    // IWRAM
    else if (address >= 0x03000000 && address < 0x03008000) {
        iwram[address - 0x03000000] = value;
    }
    // I/O Registers
    else if (address >= 0x04000000 && address < 0x04000400) {
        io_registers[address - 0x04000000] = value;
    }
    // Palette RAM
    else if (address >= 0x05000000 && address < 0x05000400) {
        palette_ram[address - 0x05000000] = value;
    }
    // VRAM
    else if (address >= 0x06000000 && address < 0x06018000) {
        vram[address - 0x06000000] = value;
    }
    // OAM
    else if (address >= 0x07000000 && address < 0x07000400) {
        oam[address - 0x07000000] = value;
    }
    // SRAM
    else if (address >= 0x0E000000 && address < 0x0E010000) {
        sram[address - 0x0E000000] = value;
    }
}

void mmu_destroy(void) {
    if (rom) {
        free(rom);
        rom = NULL;
    }
}

// TODO: Implement wait states for accurate timing
// TODO: Implement BIOS loading (optional)
