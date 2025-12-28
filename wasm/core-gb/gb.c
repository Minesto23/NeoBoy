/**
 * NeoBoy - Game Boy Main Core Implementation
 * 
 * This file implements the main Game Boy emulator core that integrates
 * all components (CPU, PPU, MMU, APU, Cartridge) and provides the
 * WASM API interface.
 */

#include "core.h"
#include "cpu.h"
#include "ppu.h"
#include "mmu.h"
#include "apu.h"
#include "cartridge.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

/* Global emulator state */
typedef struct {
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_mmu_t mmu;
    gb_apu_t apu;
    gb_cartridge_t cart;
    
    bool running;
    bool cgb_mode; /* New: CGB Mode Flag */
    uint32_t frame_count;
} gb_state_t;

static gb_state_t *gb = NULL;

void gb_init(void) {
    if (gb != NULL) {
        gb_destroy();
    }
    
    gb = (gb_state_t*)malloc(sizeof(gb_state_t));
    if (gb == NULL) {
        printf("[NeoBoy] [ERROR] Failed to allocate global state!\n");
        return;
    }
    memset(gb, 0, sizeof(gb_state_t));
    
    /* Initialize all components */
    gb_cpu_init(&gb->cpu);
    gb_ppu_init(&gb->ppu);
    gb_apu_init(&gb->apu);
    gb_cart_init(&gb->cart);
    gb_mmu_init(&gb->mmu, &gb->ppu, &gb->apu, &gb->cart);
    
    gb->running = false;
    gb->cgb_mode = false;
    gb->frame_count = 0;
    printf("[NeoBoy] Core initialized\n");
}

int gb_load_rom(const uint8_t* rom_data, uint32_t size) {
    if (gb == NULL) {
        gb_init();
    }
    
    if (gb == NULL) return -1;
    
    printf("[NeoBoy] Loading ROM: %u bytes at %p\n", size, rom_data);
    if(size >= 0x150) {
        /* Detect CGB Mode */
        u8 cgb_flag = rom_data[0x143];
        if (cgb_flag == 0x80 || cgb_flag == 0xC0) {
            gb->cgb_mode = true;
            printf("[NeoBoy] CGB Mode Detected (Flag: %02X)\n", cgb_flag);
        } else {
            gb->cgb_mode = false;
            printf("[NeoBoy] DMG Mode Detected (Flag: %02X)\n", cgb_flag);
        }
    }
    
    int result = gb_cart_load(&gb->cart, rom_data, size);
    if (result == 0) {
        gb_reset();
        gb->running = true;
        printf("[NeoBoy] ROM loaded successfully\n");
    } else {
        printf("[NeoBoy] [ERROR] ROM load failed: %d\n", result);
    }
    
    return result;
}

static int trace_count = 0;

void gb_reset(void) {
    if (gb == NULL) {
        return;
    }
    
    gb_cpu_reset(&gb->cpu);
    gb_ppu_reset(&gb->ppu);
    gb_apu_reset(&gb->apu);
    gb_mmu_reset(&gb->mmu);
    
    /* CGB Specific Initialization */
    if (gb->cgb_mode) {
        gb->cpu.a = 0x11;
        gb->cpu.b = 0x00;
        /* TODO: Set other CGB defaults if necessary (e.g. initial palette data) */
    } else {
        gb->cpu.a = 0x01; /* DMG */
        gb->cpu.b = 0x00;
    }
    
    gb->frame_count = 0;
    trace_count = 0; /* Reset trace on reset */
}
void gb_step_frame(void) {
    if (gb == NULL || !gb->running) {
        return;
    }
    
    uint32_t CYCLES_PER_FRAME = 70224;
    
    /* Double Speed Mode check */
    if (gb->mmu.speed) {
        CYCLES_PER_FRAME = 140448;
    }

    uint32_t frame_cycles = 0;
    
    while (frame_cycles < CYCLES_PER_FRAME) {
        /* High-detail trace for first few steps */
        if (trace_count < 200) {
            u8 op = gb_mmu_read(&gb->mmu, gb->cpu.pc);
            printf("[TRACE-%d] PC: 0x%04X, SP: 0x%04X, Op: 0x%02X, A: 0x%02X, F: 0x%02X\n", 
                   trace_count, gb->cpu.pc, gb->cpu.sp, op, gb->cpu.a, gb->cpu.f);
            trace_count++;
        }

        /* Step CPU */
        uint32_t cpu_cycles = gb_cpu_step(&gb->cpu, &gb->mmu);
        
        /* Calculate component cycles */
        /* PPU always runs at 4MHz. If CPU is 8MHz (Double Speed), PPU sees half cycles. */
        uint32_t ppu_cycles = gb->mmu.speed ? (cpu_cycles >> 1) : cpu_cycles;
        if (ppu_cycles == 0 && cpu_cycles > 0) ppu_cycles = 1; /* Safety minimum */
        
        /* Step PPU */
        bool vblank = gb_ppu_step(&gb->ppu, &gb->mmu, ppu_cycles);
        
        /* Step APU */
        /* APU sample generation should stay consistent? Assuming 4MHz base for now. */
        gb_apu_step(&gb->apu, ppu_cycles);
        
        /* Step Timers */
        /* Timers run at system clock (so they run faster in double speed) */
        gb_mmu_step_timers(&gb->mmu, cpu_cycles);
        
        /* Handle interrupts and catch extra cycles */
        uint32_t int_cycles = gb_cpu_handle_interrupts(&gb->cpu, &gb->mmu);
        if (int_cycles > 0) {
            uint32_t int_ppu_cycles = gb->mmu.speed ? (int_cycles >> 1) : int_cycles;
            gb_ppu_step(&gb->ppu, &gb->mmu, int_ppu_cycles);
            gb_apu_step(&gb->apu, int_ppu_cycles);
            gb_mmu_step_timers(&gb->mmu, int_cycles);
            cpu_cycles += int_cycles;
        }
        
        frame_cycles += cpu_cycles;
        
        /* Exit on VBlank */
        if (vblank) {
            break;
        }
    }
    
    if (gb->frame_count % 60 == 0) {
        u8 vram_sample1 = gb->ppu.vram[0x0000]; // 0x8000
        u8 vram_sample2 = gb->ppu.vram[0x1800]; // 0x9800
        printf("[NeoBoy] Status | Frame: %u | PC: 0x%04X | SP: 0x%04X | LY: %3u | LCDC: 0x%02X | STAT: 0x%02X | BGP: 0x%02X | VRAM[8000]: 0x%02X | VRAM[9800]: 0x%02X\n", 
               gb->frame_count, gb->cpu.pc, gb->cpu.sp, gb->ppu.ly, gb->ppu.lcdc, gb->ppu.stat, gb->ppu.bgp, vram_sample1, vram_sample2);
        fflush(stdout);
    }
    
    /* Update Cartridge (RTC) */
    gb_cart_step(&gb->cart, frame_cycles);

    gb->frame_count++;
}

void gb_set_button(GameBoyButton button, bool pressed) {
    if (gb == NULL) {
        return;
    }
    
    /* Update joypad state in MMU */
    if (pressed) {
        gb->mmu.joypad &= ~(1 << button);
    } else {
        gb->mmu.joypad |= (1 << button);
    }
}

uint8_t* gb_get_framebuffer(void) {
    if (gb == NULL) {
        return NULL;
    }
    
    return gb->ppu.framebuffer;
}

float* gb_get_audio_buffer(void) {
    if (gb == NULL) {
        return NULL;
    }
    
    return gb->apu.buffer;
}

uint32_t gb_get_audio_buffer_size(void) {
    return 4096;
}

uint32_t gb_save_state(uint8_t* buffer) {
    if (gb == NULL || buffer == NULL) {
        return 0;
    }
    
    uint8_t* ptr = buffer;
    
    /* 1. CPU */
    memcpy(ptr, &gb->cpu, sizeof(gb_cpu_t));
    ptr += sizeof(gb_cpu_t);
    
    /* 2. PPU */
    memcpy(ptr, &gb->ppu, sizeof(gb_ppu_t));
    ptr += sizeof(gb_ppu_t);
    
    /* 3. APU */
    memcpy(ptr, &gb->apu, sizeof(gb_apu_t));
    ptr += sizeof(gb_apu_t);
    
    /* 4. MMU (Skip pointers: ppu, cart, apu at end of struct) */
    /* Based on mmu.h structure, pointers are at the end of the struct */
    /* We'll save everything except the last 3 pointers */
    size_t mmu_save_size = offsetof(gb_mmu_t, ppu);
    memcpy(ptr, &gb->mmu, mmu_save_size);
    ptr += mmu_save_size;
    
    /* 5. Cartridge state (Skip pointers: rom, ram at start of struct) */
    size_t cart_meta_start = offsetof(gb_cartridge_t, rom_size);
    size_t cart_meta_size = sizeof(gb_cartridge_t) - cart_meta_start;
    memcpy(ptr, ((uint8_t*)&gb->cart) + cart_meta_start, cart_meta_size);
    ptr += cart_meta_size;
    
    /* 6. External RAM content */
    if (gb->cart.ram && gb->cart.ram_size > 0) {
        memcpy(ptr, gb->cart.ram, gb->cart.ram_size);
        ptr += gb->cart.ram_size;
    }
    
    /* 7. Global state */
    memcpy(ptr, &gb->frame_count, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    return (uint32_t)(ptr - buffer);
}

int gb_load_state(const uint8_t* buffer, uint32_t size) {
    if (gb == NULL || buffer == NULL) {
        return -1;
    }
    
    const uint8_t* ptr = buffer;
    
    /* 1. CPU */
    memcpy(&gb->cpu, ptr, sizeof(gb_cpu_t));
    ptr += sizeof(gb_cpu_t);
    
    /* 2. PPU */
    memcpy(&gb->ppu, ptr, sizeof(gb_ppu_t));
    ptr += sizeof(gb_ppu_t);
    
    /* 3. APU */
    memcpy(&gb->apu, ptr, sizeof(gb_apu_t));
    ptr += sizeof(gb_apu_t);
    
    /* 4. MMU (Restore metadata only) */
    size_t mmu_save_size = offsetof(gb_mmu_t, ppu);
    memcpy(&gb->mmu, ptr, mmu_save_size);
    ptr += mmu_save_size;
    
    /* 5. Cartridge metadata */
    // Note: We MUST NOT overwrite rom/ram pointers
    size_t cart_meta_start = offsetof(gb_cartridge_t, rom_size);
    size_t cart_meta_size = sizeof(gb_cartridge_t) - cart_meta_start;
    memcpy(((uint8_t*)&gb->cart) + cart_meta_start, ptr, cart_meta_size);
    ptr += cart_meta_size;
    
    /* 6. External RAM content */
    if (gb->cart.ram && gb->cart.ram_size > 0) {
        memcpy(gb->cart.ram, ptr, gb->cart.ram_size);
        ptr += gb->cart.ram_size;
    }
    
    /* 7. Global state */
    memcpy(&gb->frame_count, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    
    return 0;
}



void gb_destroy(void) {
    if (gb != NULL) {
        gb_cart_destroy(&gb->cart);
        free(gb);
        gb = NULL;
    }
}
