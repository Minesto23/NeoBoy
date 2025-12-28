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

/* Global emulator state */
typedef struct {
    gb_cpu_t cpu;
    gb_ppu_t ppu;
    gb_mmu_t mmu;
    gb_apu_t apu;
    gb_cartridge_t cart;
    
    bool running;
    uint32_t frame_count;
} gb_state_t;

static gb_state_t *gb = NULL;

void gb_init(void) {
    if (gb != NULL) {
        gb_destroy();
    }
    
    gb = (gb_state_t*)malloc(sizeof(gb_state_t));
    memset(gb, 0, sizeof(gb_state_t));
    
    /* Initialize all components */
    gb_cpu_init(&gb->cpu);
    gb_ppu_init(&gb->ppu);
    gb_apu_init(&gb->apu);
    gb_cart_init(&gb->cart);
    gb_mmu_init(&gb->mmu, &gb->ppu, &gb->cart);
    
    gb->running = false;
    gb->frame_count = 0;
}

int gb_load_rom(const uint8_t* rom_data, uint32_t size) {
    if (gb == NULL) {
        gb_init();
    }
    
    int result = gb_cart_load(&gb->cart, rom_data, size);
    if (result == 0) {
        gb_reset();
        gb->running = true;
    }
    
    return result;
}

void gb_step_frame(void) {
    if (gb == NULL || !gb->running) {
        return;
    }
    
    /* 
     * One frame = ~70224 cycles (4.194304 MHz / 59.73 Hz)
     * We step until VBlank is reached
     */
    const uint32_t CYCLES_PER_FRAME = 70224;
    uint32_t frame_cycles = 0;
    
    while (frame_cycles < CYCLES_PER_FRAME) {
        /* Step CPU */
        uint32_t cpu_cycles = gb_cpu_step(&gb->cpu, &gb->mmu);
        
        /* Step PPU */
        bool vblank = gb_ppu_step(&gb->ppu, cpu_cycles);
        
        /* Step APU */
        gb_apu_step(&gb->apu, cpu_cycles);
        
        /* Handle interrupts */
        gb_cpu_handle_interrupts(&gb->cpu, &gb->mmu);
        
        frame_cycles += cpu_cycles;
        
        /* Exit on VBlank */
        if (vblank) {
            break;
        }
    }
    
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

uint32_t gb_save_state(uint8_t* buffer) {
    if (gb == NULL || buffer == NULL) {
        return 0;
    }
    
    /* 
     * PLACEHOLDER: Full save state implementation
     * Should serialize: CPU, PPU, MMU, APU, Cartridge state
     */
    memcpy(buffer, gb, sizeof(gb_state_t));
    return sizeof(gb_state_t);
}

int gb_load_state(const uint8_t* buffer, uint32_t size) {
    if (gb == NULL || buffer == NULL || size != sizeof(gb_state_t)) {
        return -1;
    }
    
    /* 
     * PLACEHOLDER: Full state loading
     * Should deserialize and validate state
     */
    memcpy(gb, buffer, sizeof(gb_state_t));
    return 0;
}

void gb_reset(void) {
    if (gb == NULL) {
        return;
    }
    
    gb_cpu_reset(&gb->cpu);
    gb_ppu_reset(&gb->ppu);
    gb_apu_reset(&gb->apu);
    gb_mmu_reset(&gb->mmu);
    
    gb->frame_count = 0;
}

void gb_destroy(void) {
    if (gb != NULL) {
        gb_cart_destroy(&gb->cart);
        free(gb);
        gb = NULL;
    }
}
