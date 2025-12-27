/**
 * NeoBoy - Game Boy Color PPU
 * 
 * Extends GB PPU with:
 * - 32768 color support (15-bit RGB555)
 * - Background and sprite palettes (8 palettes each)
 * - Color palette RAM
 * - Per-tile palette selection
 */

#include "core.h"
#include <string.h>

static uint8_t framebuffer[GBC_FRAMEBUFFER_SIZE];

// Color palettes
static uint16_t bg_palettes[8][4];    // 8 background palettes, 4 colors each
static uint16_t obj_palettes[8][4];   // 8 object/sprite palettes

void ppu_init(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(bg_palettes, 0, sizeof(bg_palettes));
    memset(obj_palettes, 0, sizeof(obj_palettes));
}

void ppu_reset(void) {
    ppu_init();
}

void ppu_step(int cycles) {
    // TODO: Implement scanline rendering with color support
}

/**
 * Convert RGB555 to RGBA8888
 */
static uint32_t rgb555_to_rgba(uint16_t color) {
    uint8_t r = ((color >>  0) & 0x1F) << 3;
    uint8_t g = ((color >>  5) & 0x1F) << 3;
    uint8_t b = ((color >> 10) & 0x1F) << 3;
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

uint8_t* ppu_get_framebuffer(void) {
    return framebuffer;
}

// TODO: Implement palette RAM access (BCPS/BCPD, OCPS/OCPD)
// TODO: Implement color rendering with palette selection
// TODO: Implement priority between BG and sprites with color
