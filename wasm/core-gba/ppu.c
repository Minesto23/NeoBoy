/**
 * NeoBoy - Game Boy Advance PPU
 * 
 * GBA graphics system
 * - 240x160 display
 * - 15-bit color (RGB555): 32768 colors
 * - Multiple display modes:
 *   - Mode 0: 4 tile layers
 *   - Mode 1: 2 tile layers + 1 affine layer
 *   - Mode 2: 2 affine layers
 *   - Mode 3: Bitmap 240x160 (16-bit color)
 *   - Mode 4: Bitmap 240x160 (8-bit indexed)
 *   - Mode 5: Bitmap 160x128 (16-bit color)
 */

#include "core.h"
#include <string.h>

static uint8_t framebuffer[GBA_FRAMEBUFFER_SIZE];

typedef struct {
    int mode;
    int scanline;
    int cycles;
    uint16_t dispcnt;  // Display control register
} GBAPPU;

static GBAPPU ppu;

void ppu_init(void) {
    memset(&ppu, 0, sizeof(GBAPPU));
    memset(framebuffer, 0, sizeof(framebuffer));
}

void ppu_reset(void) {
    ppu_init();
}

/**
 * Convert RGB555 to RGBA8888
 */
static void set_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= GBA_SCREEN_WIDTH || y < 0 || y >= GBA_SCREEN_HEIGHT) {
        return;
    }
    
    uint8_t r = ((color >>  0) & 0x1F) << 3;
    uint8_t g = ((color >>  5) & 0x1F) << 3;
    uint8_t b = ((color >> 10) & 0x1F) << 3;
    
    int index = (y * GBA_SCREEN_WIDTH + x) * 4;
    framebuffer[index + 0] = r;
    framebuffer[index + 1] = g;
    framebuffer[index + 2] = b;
    framebuffer[index + 3] = 0xFF;
}

void ppu_step(int cycles) {
    ppu.cycles += cycles;
    
    // Scanline timing: 960 cycles per line, 228 cycles per scanline
    if (ppu.cycles >= 960) {
        ppu.cycles -= 960;
        ppu.scanline++;
        
        if (ppu.scanline < 160) {
            // Render scanline
            ppu_render_scanline();
        }
        
        if (ppu.scanline == 160) {
            // VBlank start
            // TODO: Trigger VBlank interrupt
        }
        
        if (ppu.scanline >= 228) {
            ppu.scanline = 0;
        }
    }
}

/**
 * Render a scanline based on current display mode
 */
void ppu_render_scanline(void) {
    int bg_mode = ppu.dispcnt & 0x07;
    int y = ppu.scanline;
    
    // Placeholder: render white screen
    for (int x = 0; x < GBA_SCREEN_WIDTH; x++) {
        set_pixel(x, y, 0x7FFF);  // White
    }
    
    // TODO: Implement rendering for each mode
}

uint8_t* ppu_get_framebuffer(void) {
    return framebuffer;
}

void ppu_set_dispcnt(uint16_t value) {
    ppu.dispcnt = value;
}

// TODO: Implement Mode 0-2 (tile-based)
// TODO: Implement Mode 3-5 (bitmap modes)
// TODO: Implement sprite rendering
// TODO: Implement affine transformations
// TODO: Implement blending and special effects
