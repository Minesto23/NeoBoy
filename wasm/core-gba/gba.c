/**
 * NeoBoy - Game Boy Advance Main Core Implementation
 */

#include "core.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations
void cpu_init(void);
void cpu_reset(void);
int cpu_step(void);
void mmu_init(void);
int mmu_load_rom(const uint8_t* data, uint32_t size);
void mmu_destroy(void);

void gba_init(void) {
    cpu_init();
    mmu_init();
}

int gba_load_rom(const uint8_t* rom_data, uint32_t size) {
    return mmu_load_rom(rom_data, size);
}

void gba_step_frame(void) {
    // Execute cycles for one frame
    // GBA clock is 16.78 MHz, 60fps => ~280k cycles per frame
    const uint32_t CYCLES_PER_FRAME = 280000;
    uint32_t cycles = 0;
    while (cycles < CYCLES_PER_FRAME) {
        cycles += cpu_step();
    }
}

void gba_set_button(GBAButton button, bool pressed) {
    // TODO: Implement GBA input
}

uint8_t* gba_get_framebuffer(void) {
    // TODO: Return GBA framebuffer
    return NULL;
}

uint32_t gba_save_state(uint8_t* buffer) {
    return 0;
}

int gba_load_state(const uint8_t* buffer, uint32_t size) {
    return 0;
}

void gba_reset(void) {
    cpu_reset();
}

void gba_destroy(void) {
    mmu_destroy();
}
