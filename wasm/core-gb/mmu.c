/**
 * NeoBoy - Game Boy Memory Management Unit (MMU)
 * 
 * Memory Map:
 * 0x0000-0x3FFF: ROM Bank 0 (16KB)
 * 0x4000-0x7FFF: ROM Bank N (switchable, 16KB)
 * 0x8000-0x9FFF: Video RAM (8KB)
 * 0xA000-0xBFFF: External RAM (8KB, from cartridge)
 * 0xC000-0xDFFF: Work RAM (8KB)
 * 0xE000-0xFDFF: Echo RAM (mirror of 0xC000-0xDDFF)
 * 0xFE00-0xFE9F: OAM (Object Attribute Memory)
 * 0xFEA0-0xFEFF: Unusable
 * 0xFF00-0xFF7F: I/O Registers
 * 0xFF80-0xFFFE: High RAM (HRAM)
 * 0xFFFF: Interrupt Enable Register
 */

#include "core.h"
#include <stdlib.h>
#include <string.h>

// Memory regions
static uint8_t* rom_bank0;           // 0x0000-0x3FFF (16KB)
static uint8_t* rom_bank_n;          // 0x4000-0x7FFF (16KB, switchable)
static uint8_t vram[0x2000];         // 0x8000-0x9FFF (8KB)
static uint8_t* external_ram;        // 0xA000-0xBFFF (8KB max)
static uint8_t wram[0x2000];         // 0xC000-0xDFFF (8KB)
static uint8_t oam[0xA0];            // 0xFE00-0xFE9F (160 bytes)
static uint8_t io_registers[0x80];   // 0xFF00-0xFF7F
static uint8_t hram[0x7F];           // 0xFF80-0xFFFE
static uint8_t interrupt_enable;     // 0xFFFF

static uint8_t* rom_data = NULL;
static uint32_t rom_size = 0;
static int current_rom_bank = 1;

void mmu_init(void) {
    memset(vram, 0, sizeof(vram));
    memset(wram, 0, sizeof(wram));
    memset(oam, 0, sizeof(oam));
    memset(io_registers, 0, sizeof(io_registers));
    memset(hram, 0, sizeof(hram));
    interrupt_enable = 0;
    current_rom_bank = 1;
}

int mmu_load_rom(const uint8_t* data, uint32_t size) {
    if (data == NULL || size < 0x8000) {
        return -1;  // Invalid ROM
    }
    
    // Free previous ROM if exists
    if (rom_data != NULL) {
        free(rom_data);
    }
    
    // Allocate and copy ROM data
    rom_data = (uint8_t*)malloc(size);
    if (rom_data == NULL) {
        return -1;
    }
    
    memcpy(rom_data, data, size);
    rom_size = size;
    
    // Set up bank pointers
    rom_bank0 = rom_data;
    rom_bank_n = rom_data + 0x4000;
    
    return 0;
}

uint8_t mmu_read(uint16_t address) {
    // ROM Bank 0
    if (address < 0x4000) {
        return rom_bank0 ? rom_bank0[address] : 0xFF;
    }
    // ROM Bank N
    else if (address < 0x8000) {
        return rom_bank_n ? rom_bank_n[address - 0x4000] : 0xFF;
    }
    // VRAM
    else if (address < 0xA000) {
        return vram[address - 0x8000];
    }
    // External RAM
    else if (address < 0xC000) {
        return external_ram ? external_ram[address - 0xA000] : 0xFF;
    }
    // WRAM
    else if (address < 0xE000) {
        return wram[address - 0xC000];
    }
    // Echo RAM
    else if (address < 0xFE00) {
        return wram[address - 0xE000];
    }
    // OAM
    else if (address < 0xFEA0) {
        return oam[address - 0xFE00];
    }
    // Unusable
    else if (address < 0xFF00) {
        return 0xFF;
    }
    // I/O Registers
    else if (address < 0xFF80) {
        return io_registers[address - 0xFF00];
    }
    // HRAM
    else if (address < 0xFFFF) {
        return hram[address - 0xFF80];
    }
    // Interrupt Enable
    else {
        return interrupt_enable;
    }
}

void mmu_write(uint16_t address, uint8_t value) {
    // ROM (MBC writes - banking control)
    if (address < 0x8000) {
        // TODO: Handle MBC (Memory Bank Controller) writes
        return;
    }
    // VRAM
    else if (address < 0xA000) {
        vram[address - 0x8000] = value;
    }
    // External RAM
    else if (address < 0xC000) {
        if (external_ram) {
            external_ram[address - 0xA000] = value;
        }
    }
    // WRAM
    else if (address < 0xE000) {
        wram[address - 0xC000] = value;
    }
    // Echo RAM
    else if (address < 0xFE00) {
        wram[address - 0xE000] = value;
    }
    // OAM
    else if (address < 0xFEA0) {
        oam[address - 0xFE00] = value;
    }
    // Unusable
    else if (address < 0xFF00) {
        return;
    }
    // I/O Registers
    else if (address < 0xFF80) {
        io_registers[address - 0xFF00] = value;
    }
    // HRAM
    else if (address < 0xFFFF) {
        hram[address - 0xFF80] = value;
    }
    // Interrupt Enable
    else {
        interrupt_enable = value;
    }
}

void mmu_destroy(void) {
    if (rom_data != NULL) {
        free(rom_data);
        rom_data = NULL;
    }
}

// TODO: Implement MBC (Memory Bank Controller) support
// - MBC1, MBC2, MBC3, MBC5
// - ROM and RAM banking
