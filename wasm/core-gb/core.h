/**
 * NeoBoy - Game Boy (DMG) Core Header
 * 
 * This header defines the public API for the Game Boy core
 * exposed to WebAssembly and the frontend.
 * 
 * Display: 160x144 pixels, 4 shades of gray
 * CPU: Sharp LR35902 (8-bit, similar to Z80)
 */

#ifndef NEOBOY_GB_CORE_H
#define NEOBOY_GB_CORE_H

#include <stdint.h>
#include <stdbool.h>

// Display constants
#define GB_SCREEN_WIDTH 160
#define GB_SCREEN_HEIGHT 144
#define GB_FRAMEBUFFER_SIZE (GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT * 4) // RGBA

// Button mapping
typedef enum {
    BTN_A      = 0,
    BTN_B      = 1,
    BTN_SELECT = 2,
    BTN_START  = 3,
    BTN_RIGHT  = 4,
    BTN_LEFT   = 5,
    BTN_UP     = 6,
    BTN_DOWN   = 7
} GameBoyButton;

// ===== WASM Exported Functions =====

/**
 * Initialize the Game Boy core
 */
void gb_init(void);

/**
 * Load ROM from memory buffer
 * @param rom_data Pointer to ROM data
 * @param size Size of ROM in bytes
 * @return 0 on success, -1 on failure
 */
int gb_load_rom(const uint8_t* rom_data, uint32_t size);

/**
 * Execute one frame (approximately 70224 cycles)
 * Advances emulation to the next VBlank
 */
void gb_step_frame(void);

/**
 * Set button state
 * @param button Button identifier
 * @param pressed true if pressed, false if released
 */
void gb_set_button(GameBoyButton button, bool pressed);

/**
 * Get pointer to framebuffer (RGBA format)
 * @return Pointer to framebuffer array
 */
uint8_t* gb_get_framebuffer(void);

/**
 * Save emulator state
 * @param buffer Output buffer for state data
 * @return Size of saved state in bytes
 */
uint32_t gb_save_state(uint8_t* buffer);

/**
 * Load emulator state
 * @param buffer Input buffer containing state data
 * @param size Size of state data
 * @return 0 on success, -1 on failure
 */
int gb_load_state(const uint8_t* buffer, uint32_t size);

/**
 * Reset the emulator
 */
void gb_reset(void);

/**
 * Clean up and free resources
 */
void gb_destroy(void);

#endif // NEOBOY_GB_CORE_H
