/**
 * NeoBoy - Game Boy Color CPU
 * 
 * Same LR35902 as GB but with:
 * - Double-speed mode (8.388608 MHz)
 * - CGB-specific registers
 * - Enhanced timing
 */

#include "core.h"
#include <string.h>

// CPU state (extends GB CPU)
// TODO: Implement double-speed mode switching
// TODO: Add CGB-specific flags

void cpu_init(void) {
    // TODO: Initialize CPU with CGB boot values
}

void cpu_reset(void) {
    cpu_init();
}

int cpu_step(void) {
    // TODO: Implement with double-speed support
    return 4;
}

// TODO: Implement KEY1 register for speed switching
