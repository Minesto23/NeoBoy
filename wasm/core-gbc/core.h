/**
 * NeoBoy - Game Boy Color Core Header
 * 
 * Extends Game Boy with color support
 * - Full color display (32768 colors)
 * - VRAM banking (2 banks)
 * - Double-speed CPU mode
 * - Color palettes for background and sprites
 */

#ifndef NEOBOY_GBC_CORE_H
#define NEOBOY_GBC_CORE_H

#include <stdint.h>
#include <stdbool.h>

// Display constants
#define GBC_SCREEN_WIDTH 160
#define GBC_SCREEN_HEIGHT 144
#define GBC_FRAMEBUFFER_SIZE (GBC_SCREEN_WIDTH * GBC_SCREEN_HEIGHT * 4) // RGBA

// Button mapping (same as GB)
typedef enum {
    BTN_A      = 0,
    BTN_B      = 1,
    BTN_SELECT = 2,
    BTN_START  = 3,
    BTN_RIGHT  = 4,
    BTN_LEFT   = 5,
    BTN_UP     = 6,
    BTN_DOWN   = 7
} GameBoyColorButton;

// ===== WASM Exported Functions =====

void gbc_init(void);
int gbc_load_rom(const uint8_t* rom_data, uint32_t size);
void gbc_step_frame(void);
void gbc_set_button(GameBoyColorButton button, bool pressed);
uint8_t* gbc_get_framebuffer(void);
uint32_t gbc_save_state(uint8_t* buffer);
int gbc_load_state(const uint8_t* buffer, uint32_t size);
void gbc_reset(void);
void gbc_destroy(void);

#endif // NEOBOY_GBC_CORE_H
