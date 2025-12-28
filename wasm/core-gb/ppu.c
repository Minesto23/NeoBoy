/**
 * NeoBoy - Game Boy PPU Implementation
 * 
 * Purpose: Graphics rendering and LCD control
 * 
 * PLACEHOLDER: This implementation provides the basic structure but needs:
 * - Tile fetcher and pixel FIFO
 * - Sprite rendering with priority
 * - Window layer rendering
 * - Accurate timing and mode transitions
 * - STAT interrupt generation
 */

#include "ppu.h"
#include <string.h>
#include <stdio.h>

/* Default monochrome palette (darkest to lightest) */
static const u32 default_palette[4] = {
    0xFF0F380F,  /* Color 0: Darkest green */
    0xFF0E6C28,  /* Color 1: Dark green */
    0xFF2A9A39,  /* Color 2: Light green */
    0xFF8BBE53   /* Color 3: Lightest green */
};

void gb_ppu_init(gb_ppu_t *ppu) {
    memset(ppu, 0, sizeof(gb_ppu_t));
    
    /* Default register values */
    ppu->lcdc = 0x91;
    ppu->stat = 0x00;
    ppu->bgp = 0xFC;
    ppu->obp0 = 0xFF;
    ppu->obp1 = 0xFF;
    
    ppu->mode = PPU_MODE_OAM_SCAN;
    ppu->mode_cycles = 0;
    
    /* Clear framebuffer to black */
    for (u32 i = 0; i < sizeof(ppu->framebuffer); i += 4) {
        ppu->framebuffer[i + 0] = 0x00;
        ppu->framebuffer[i + 1] = 0x00;
        ppu->framebuffer[i + 2] = 0x00;
        ppu->framebuffer[i + 3] = 0xFF;
    }
}

void gb_ppu_reset(gb_ppu_t *ppu) {
    gb_ppu_init(ppu);
}

bool gb_ppu_step(gb_ppu_t *ppu, u32 cycles) {
    if (!(ppu->lcdc & LCDC_ENABLE)) {
        return false;
    }
    
    ppu->mode_cycles += cycles;
    bool frame_complete = false;
    
    /*
     * PLACEHOLDER: Simplified mode state machine
     * Real implementation needs accurate cycle timing
     */
    switch (ppu->mode) {
        case PPU_MODE_OAM_SCAN:
            if (ppu->mode_cycles >= 80) {
                ppu->mode = PPU_MODE_DRAWING;
                ppu->mode_cycles -= 80;
            }
            break;
            
        case PPU_MODE_DRAWING:
            if (ppu->mode_cycles >= 172) {
                ppu->mode = PPU_MODE_HBLANK;
                ppu->mode_cycles -= 172;
                gb_ppu_render_scanline(ppu);
            }
            break;
            
        case PPU_MODE_HBLANK:
            if (ppu->mode_cycles >= 204) {
                ppu->mode_cycles -= 204;
                ppu->ly++;
                
                if (ppu->ly >= 144) {
                    ppu->mode = PPU_MODE_VBLANK;
                    frame_complete = true;
                } else {
                    ppu->mode = PPU_MODE_OAM_SCAN;
                }
            }
            break;
            
        case PPU_MODE_VBLANK:
            if (ppu->mode_cycles >= 456) {
                ppu->mode_cycles -= 456;
                ppu->ly++;
                
                if (ppu->ly >= 154) {
                    ppu->ly = 0;
                    ppu->mode = PPU_MODE_OAM_SCAN;
                }
            }
            break;
    }
    
    if (frame_complete) {
        static int frames = 0;
        if (frames++ % 60 == 0) {
            printf("PPU Frame Complete (1/60)\n");
        }
    }
    
    return frame_complete;
}

void gb_ppu_render_scanline(gb_ppu_t *ppu) {
    /*
     * PLACEHOLDER: Scanline rendering
     * 
     * Full implementation would:
     * 1. Render background tiles for current scanline
     * 2. Render window if enabled
     * 3. Render sprites with priority handling
     * 4. Apply palette mapping
     * 5. Write pixels to framebuffer
     */
    
    u32 y = ppu->ly;
    if (y >= GB_SCREEN_HEIGHT) return;
    
    /* Render background tiles if enabled */
    if (!(ppu->lcdc & LCDC_ENABLE)) return;
    if (!(ppu->lcdc & LCDC_BG_ENABLE)) return;

    /* 
     * Simplified tile rendering logic:
     * 1. Find the tile map address (0x9800 or 0x9C00)
     * 2. Find the tile data address (0x8000 or 0x8800)
     * 3. For each pixel, fetch tile index and pixel color
     */
    u16 map_base = (ppu->lcdc & LCDC_BG_TILEMAP) ? 0x1C00 : 0x1800; // Relative to 0x8000
    u16 tile_base = (ppu->lcdc & LCDC_BG_WIN_TILES) ? 0x0000 : 0x0800;

    u8 ty = (y + ppu->scy) / 8;
    u8 py = (y + ppu->scy) % 8;

    for (u32 x = 0; x < GB_SCREEN_WIDTH; x++) {
        u8 tx = (x + ppu->scx) / 8;
        u8 px = (x + ppu->scx) % 8;

        /* Fetch tile index from map */
        u16 map_offset = map_base + (ty % 32) * 32 + (tx % 32);
        u8 tile_index = ppu->vram[map_offset];

        /* Fetch tile data pixels */
        u16 tile_addr;
        if (ppu->lcdc & LCDC_BG_WIN_TILES) {
            tile_addr = tile_base + tile_index * 16 + py * 2;
        } else {
            tile_addr = tile_base + (int8_t)tile_index * 16 + py * 2;
        }

        u8 b1 = ppu->vram[tile_addr];
        u8 b2 = ppu->vram[tile_addr + 1];

        /* Combine bits for 2-bit color index */
        u8 bit = 7 - px;
        u8 color_idx = ((b1 >> bit) & 1) | (((b2 >> bit) & 1) << 1);

        /* Map to palette and write to framebuffer */
        u8 shade = (ppu->bgp >> (color_idx * 2)) & 3;
        u32 color = default_palette[shade];
        
        u32 fb_idx = (y * GB_SCREEN_WIDTH + x) * 4;
        ppu->framebuffer[fb_idx + 0] = (color >> 0) & 0xFF;
        ppu->framebuffer[fb_idx + 1] = (color >> 8) & 0xFF;
        ppu->framebuffer[fb_idx + 2] = (color >> 16) & 0xFF;
        ppu->framebuffer[fb_idx + 3] = (color >> 24) & 0xFF;
    }
}

void gb_ppu_write_vram(gb_ppu_t *ppu, u16 addr, u8 value) {
    if (addr < 0x2000) {
        ppu->vram[addr] = value;
    }
}

u8 gb_ppu_read_vram(gb_ppu_t *ppu, u16 addr) {
    if (addr < 0x2000) {
        return ppu->vram[addr];
    }
    return 0xFF;
}

void gb_ppu_write_oam(gb_ppu_t *ppu, u16 addr, u8 value) {
    if (addr < 0xA0) {
        ppu->oam[addr] = value;
    }
}

u8 gb_ppu_read_oam(gb_ppu_t *ppu, u16 addr) {
    if (addr < 0xA0) {
        return ppu->oam[addr];
    }
    return 0xFF;
}
