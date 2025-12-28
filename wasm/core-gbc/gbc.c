/**
 * NeoBoy - Game Boy Color Main Core Implementation
 */

#include "core.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations of component functions
void cpu_init(void);
void cpu_reset(void);
int cpu_step(void);
void mmu_init(void);
void mmu_reset(void); // Placeholder if not in mmu.c

void gbc_init(void) {
    cpu_init();
    mmu_init();
}

int gbc_load_rom(const uint8_t* rom_data, uint32_t size) {
    // TODO: Implement ROM loading for GBC
    return 0;
}

void gbc_step_frame(void) {
    // Execute cycles for one frame
    const uint32_t CYCLES_PER_FRAME = 70224 * 2; // Double speed potentially
    uint32_t cycles = 0;
    while (cycles < CYCLES_PER_FRAME) {
        cycles += cpu_step();
    }
}

void gbc_set_button(GameBoyColorButton button, bool pressed) {
    // TODO: Implement joypad for GBC
}

uint8_t* gbc_get_framebuffer(void) {
    // TODO: Return pointer to GBC VRAM or rendered buffer
    return NULL;
}

uint32_t gbc_save_state(uint8_t* buffer) {
    return 0;
}

int gbc_load_state(const uint8_t* buffer, uint32_t size) {
    return 0;
}

void gbc_reset(void) {
    cpu_reset();
}

void gbc_destroy(void) {
    // Cleanup
}
