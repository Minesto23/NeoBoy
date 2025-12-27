/**
 * NeoBoy - Game Boy Advance Core Header
 * 
 * ARM7TDMI-based handheld console
 * - 240x160 display, 15-bit color (RGB555)
 * - ARM7TDMI CPU (16.78 MHz)
 * - Multiple graphics modes
 * - DMA, timers, interrupts
 */

#ifndef NEOBOY_GBA_CORE_H
#define NEOBOY_GBA_CORE_H

#include <stdint.h>
#include <stdbool.h>

// Display constants
#define GBA_SCREEN_WIDTH 240
#define GBA_SCREEN_HEIGHT 160
#define GBA_FRAMEBUFFER_SIZE (GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * 4) // RGBA

// Button mapping
typedef enum {
    BTN_A      = 0,
    BTN_B      = 1,
    BTN_SELECT = 2,
    BTN_START  = 3,
    BTN_RIGHT  = 4,
    BTN_LEFT   = 5,
    BTN_UP     = 6,
    BTN_DOWN   = 7,
    BTN_L      = 8,
    BTN_R      = 9
} GBAButton;

// ===== WASM Exported Functions =====

void gba_init(void);
int gba_load_rom(const uint8_t* rom_data, uint32_t size);
void gba_step_frame(void);
void gba_set_button(GBAButton button, bool pressed);
uint8_t* gba_get_framebuffer(void);
uint32_t gba_save_state(uint8_t* buffer);
int gba_load_state(const uint8_t* buffer, uint32_t size);
void gba_reset(void);
void gba_destroy(void);

#endif // NEOBOY_GBA_CORE_H
