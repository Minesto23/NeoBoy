/**
 * NeoBoy - Game Boy Pixel Processing Unit (PPU)
 * 
 * Handles rendering of graphics
 * - 160x144 pixel display
 * - 4 shades of gray (2-bit color)
 * - Background, Window, and Sprite layers
 * - Scanline-based rendering
 * 
 * PPU Modes:
 * - Mode 0: HBlank
 * - Mode 1: VBlank
 * - Mode 2: OAM Search
 * - Mode 3: Drawing
 */

#include "core.h"
#include <string.h>

// Framebuffer (RGBA format for web display)
static uint8_t framebuffer[GB_FRAMEBUFFER_SIZE];

// PPU state
typedef struct {
    int mode;
    int scanline;
    int cycles;
} PPU;

static PPU ppu;

// DMG palette (4 shades of green-tinted gray)
static const uint32_t DMG_PALETTE[4] = {
    0xFFE0F8D0,  // Lightest (white)
    0xFF88C070,  // Light gray
    0xFF346856,  // Dark gray
    0xFF081820   // Darkest (black)
};

void ppu_init(void) {
    memset(&ppu, 0, sizeof(PPU));
    memset(framebuffer, 0, sizeof(framebuffer));
    ppu.mode = 2;  // Start in OAM search
}

void ppu_reset(void) {
    ppu_init();
}

/**
 * Step PPU by a given number of cycles
 */
void ppu_step(int cycles) {
    ppu.cycles += cycles;
    
    switch (ppu.mode) {
        case 2:  // OAM Search (80 cycles)
            if (ppu.cycles >= 80) {
                ppu.cycles -= 80;
                ppu.mode = 3;  // Switch to Drawing
            }
            break;
            
        case 3:  // Drawing (172 cycles)
            if (ppu.cycles >= 172) {
                ppu.cycles -= 172;
                ppu.mode = 0;  // Switch to HBlank
                // TODO: Render scanline
            }
            break;
            
        case 0:  // HBlank (204 cycles)
            if (ppu.cycles >= 204) {
                ppu.cycles -= 204;
                ppu.scanline++;
                
                if (ppu.scanline == 144) {
                    ppu.mode = 1;  // Switch to VBlank
                    // TODO: Trigger VBlank interrupt
                } else {
                    ppu.mode = 2;  // Next scanline
                }
            }
            break;
            
        case 1:  // VBlank (4560 cycles total for 10 scanlines)
            if (ppu.cycles >= 456) {
                ppu.cycles -= 456;
                ppu.scanline++;
                
                if (ppu.scanline == 154) {
                    ppu.scanline = 0;
                    ppu.mode = 2;  // Back to OAM search
                }
            }
            break;
    }
}

/**
 * Render a single scanline
 * TODO: Implement background, window, and sprite rendering
 */
void ppu_render_scanline(void) {
    int y = ppu.scanline;
    
    for (int x = 0; x < GB_SCREEN_WIDTH; x++) {
        // Placeholder: white screen
        int pixel_index = (y * GB_SCREEN_WIDTH + x) * 4;
        framebuffer[pixel_index + 0] = 0xE0;  // R
        framebuffer[pixel_index + 1] = 0xF8;  // G
        framebuffer[pixel_index + 2] = 0xD0;  // B
        framebuffer[pixel_index + 3] = 0xFF;  // A
    }
}

uint8_t* ppu_get_framebuffer(void) {
    return framebuffer;
}

int ppu_get_scanline(void) {
    return ppu.scanline;
}

// TODO: Implement background rendering
// TODO: Implement window rendering
// TODO: Implement sprite rendering
// TODO: Implement color palette support
