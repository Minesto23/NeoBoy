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
#include "mmu.h"
#include <string.h>
#include <stdio.h>

/* Default monochrome palette (darkest to lightest) */
static const u32 default_palette[4] = {
    0xFF8BBE53,  /* Color 0: Lightest green */
    0xFF2A9A39,  /* Color 1: Light green */
    0xFF0E6C28,  /* Color 2: Dark green */
    0xFF0F380F   /* Color 3: Darkest green */
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

static void request_interrupt(gb_mmu_t *mmu, u8 interrupt) {
    u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
    gb_mmu_write(mmu, 0xFF0F, if_reg | interrupt);
}

static void update_stat(gb_ppu_t *ppu, gb_mmu_t *mmu) {
    u8 old_stat = ppu->stat;
    
    /* Update mode bits 0-1 */
    ppu->stat = (ppu->stat & ~STAT_MODE_MASK) | (ppu->mode & STAT_MODE_MASK);
    
    /* Update LYC=LY bit 2 */
    if (ppu->ly == ppu->lyc) {
        ppu->stat |= STAT_LYC_EQUAL;
        /* Trigger LYC interrupt if enabled and bit was just set */
        if ((ppu->stat & STAT_INTERRUPT_LYC) && !(old_stat & STAT_LYC_EQUAL)) {
            request_interrupt(mmu, 0x02); // LCD STAT interrupt
        }
    } else {
        ppu->stat &= ~STAT_LYC_EQUAL;
    }
}

bool gb_ppu_step(gb_ppu_t *ppu, void *mmu_ptr, u32 cycles) {
    gb_mmu_t *mmu = (gb_mmu_t *)mmu_ptr;
    
    if (!(ppu->lcdc & LCDC_ENABLE)) {
        ppu->ly = 0;
        ppu->mode_cycles = 0;
        ppu->mode = PPU_MODE_HBLANK;
        return false;
    }
    
    ppu->mode_cycles += cycles;
    bool frame_complete = false;
    u8 old_mode = ppu->mode;
    
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
                
                /* Trigger H-Blank STAT interrupt if enabled */
                if (ppu->stat & STAT_INTERRUPT_HBL) {
                    request_interrupt(mmu, 0x02);
                }
            }
            break;
            
        case PPU_MODE_HBLANK:
            if (ppu->mode_cycles >= 204) {
                ppu->mode_cycles -= 204;
                ppu->ly++;
                
                update_stat(ppu, mmu);
                
                if (ppu->ly >= 144) {
                    ppu->mode = PPU_MODE_VBLANK;
                    request_interrupt(mmu, 0x01); // V-Blank interrupt
                    
                    /* Trigger V-Blank STAT interrupt if enabled */
                    if (ppu->stat & STAT_INTERRUPT_VBL) {
                        request_interrupt(mmu, 0x02);
                    }
                    frame_complete = true;
                } else {
                    ppu->mode = PPU_MODE_OAM_SCAN;
                    /* Trigger OAM STAT interrupt if enabled */
                    if (ppu->stat & STAT_INTERRUPT_OAM) {
                        request_interrupt(mmu, 0x02);
                    }
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
                    /* Trigger OAM STAT interrupt if enabled */
                    if (ppu->stat & STAT_INTERRUPT_OAM) {
                        request_interrupt(mmu, 0x02);
                    }
                }
                update_stat(ppu, mmu);
            }
            break;
    }
    
    if (ppu->mode != old_mode) {
        update_stat(ppu, mmu);
    }
    
    return frame_complete;
}

void gb_ppu_render_scanline(gb_ppu_t *ppu) {
    if (!(ppu->lcdc & LCDC_ENABLE)) return;

    u8 scanline_row[GB_SCREEN_WIDTH]; /* Store color indices for priority handling */
    memset(scanline_row, 0, sizeof(scanline_row));

    /* 1. Render Background */
    if (ppu->lcdc & LCDC_BG_ENABLE) {
        u16 map_base = (ppu->lcdc & LCDC_BG_TILEMAP) ? 0x1C00 : 0x1800;
        u16 tile_base = (ppu->lcdc & LCDC_BG_WIN_TILES) ? 0x0000 : 0x1000;
        u8 ty = (ppu->ly + ppu->scy) / 8;
        u8 py = (ppu->ly + ppu->scy) % 8;

        for (u32 x = 0; x < GB_SCREEN_WIDTH; x++) {
            u8 tx = (x + ppu->scx) / 8;
            u8 px = (x + ppu->scx) % 8;
            u16 map_offset = map_base + (ty % 32) * 32 + (tx % 32);
            u8 tile_index = ppu->vram[map_offset];
            u16 tile_addr;
            if (ppu->lcdc & LCDC_BG_WIN_TILES) {
                tile_addr = tile_base + tile_index * 16 + py * 2;
            } else {
                tile_addr = tile_base + (int8_t)tile_index * 16 + py * 2;
            }
            u8 b1 = ppu->vram[tile_addr];
            u8 b2 = ppu->vram[tile_addr + 1];
            u8 bit = 7 - px;
            u8 color_idx = ((b1 >> bit) & 1) | (((b2 >> bit) & 1) << 1);
            scanline_row[x] = color_idx;
        }
    }

    /* 2. Render Window */
    if ((ppu->lcdc & LCDC_WIN_ENABLE) && ppu->ly >= ppu->wy) {
        u16 map_base = (ppu->lcdc & LCDC_WIN_TILEMAP) ? 0x1C00 : 0x1800;
        u16 tile_base = (ppu->lcdc & LCDC_BG_WIN_TILES) ? 0x0000 : 0x1000;
        u8 ty = (ppu->ly - ppu->wy) / 8;
        u8 py = (ppu->ly - ppu->wy) % 8;

        int win_x_start = (int)ppu->wx - 7;
        for (int x = win_x_start; x < GB_SCREEN_WIDTH; x++) {
            if (x < 0) continue;
            u8 tx = (x - win_x_start) / 8;
            u8 px = (x - win_x_start) % 8;
            u16 map_offset = map_base + (ty % 32) * 32 + (tx % 32);
            u8 tile_index = ppu->vram[map_offset];
            u16 tile_addr;
            if (ppu->lcdc & LCDC_BG_WIN_TILES) {
                tile_addr = tile_base + tile_index * 16 + py * 2;
            } else {
                tile_addr = tile_base + (int8_t)tile_index * 16 + py * 2;
            }
            u8 b1 = ppu->vram[tile_addr];
            u8 b2 = ppu->vram[tile_addr + 1];
            u8 bit = 7 - px;
            u8 color_idx = ((b1 >> bit) & 1) | (((b2 >> bit) & 1) << 1);
            scanline_row[x] = color_idx;
        }
    }

    /* Apply background/window palette and write to framebuffer */
    for (u32 x = 0; x < GB_SCREEN_WIDTH; x++) {
        u8 color_idx = scanline_row[x];
        u8 shade = (ppu->bgp >> (color_idx * 2)) & 3;
        u32 color = default_palette[shade];
        u32 fb_idx = (ppu->ly * GB_SCREEN_WIDTH + x) * 4;
        
        if (fb_idx + 3 < GB_FRAMEBUFFER_SIZE) {
            ppu->framebuffer[fb_idx + 0] = (color >> 0) & 0xFF;
            ppu->framebuffer[fb_idx + 1] = (color >> 8) & 0xFF;
            ppu->framebuffer[fb_idx + 2] = (color >> 16) & 0xFF;
            ppu->framebuffer[fb_idx + 3] = (color >> 24) & 0xFF;
        }
    }

    /* 3. Render Sprites (OBJ) */
    if (ppu->lcdc & LCDC_OBJ_ENABLE) {
        u8 obj_height = (ppu->lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
        int sprites_rendered = 0;

        for (int i = 0; i < 40 && sprites_rendered < 10; i++) {
            u16 oam_addr = i * 4;
            int y = (int)ppu->oam[oam_addr] - 16;
            int x = (int)ppu->oam[oam_addr + 1] - 8;
            u8 tile_index = ppu->oam[oam_addr + 2];
            u8 attr = ppu->oam[oam_addr + 3];

            if (ppu->ly >= y && ppu->ly < (y + obj_height)) {
                sprites_rendered++;
                bool flip_y = attr & (1 << 6);
                bool flip_x = attr & (1 << 5);
                bool priority = attr & (1 << 7); /* 0: OBJ above BG, 1: OBJ behind BG 1-3 */
                u8 palette = (attr & (1 << 4)) ? ppu->obp1 : ppu->obp0;

                int py = ppu->ly - y;
                if (flip_y) py = obj_height - 1 - py;

                if (obj_height == 16) tile_index &= ~0x01; /* Bit 0 ignored in 8x16 mode */

                u16 tile_addr = tile_index * 16 + py * 2;
                u8 b1 = ppu->vram[tile_addr];
                u8 b2 = ppu->vram[tile_addr + 1];

                for (int px = 0; px < 8; px++) {
                    int screen_x = x + px;
                    if (screen_x < 0 || screen_x >= GB_SCREEN_WIDTH) continue;

                    u8 bit = flip_x ? px : 7 - px;
                    u8 color_idx = ((b1 >> bit) & 1) | (((b2 >> bit) & 1) << 1);

                    if (color_idx == 0) continue; /* Transparent */

                    /* Priority check */
                    if (priority && scanline_row[screen_x] != 0) continue;

                    u8 shade = (palette >> (color_idx * 2)) & 3;
                    u32 color = default_palette[shade];
                    u32 fb_idx = (ppu->ly * GB_SCREEN_WIDTH + screen_x) * 4;
                    
                    if (fb_idx + 3 < GB_FRAMEBUFFER_SIZE) {
                        ppu->framebuffer[fb_idx + 0] = (color >> 0) & 0xFF;
                        ppu->framebuffer[fb_idx + 1] = (color >> 8) & 0xFF;
                        ppu->framebuffer[fb_idx + 2] = (color >> 16) & 0xFF;
                        ppu->framebuffer[fb_idx + 3] = (color >> 24) & 0xFF;
                    }
                }
            }
        }
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
u8 gb_ppu_read_reg(gb_ppu_t *ppu, u16 addr) {
    switch (addr) {
        case 0xFF40: return ppu->lcdc;
        case 0xFF41: return ppu->stat | 0x80; /* Bit 7 always 1 */
        case 0xFF42: return ppu->scy;
        case 0xFF43: return ppu->scx;
        case 0xFF44: return ppu->ly;
        case 0xFF45: return ppu->lyc;
        case 0xFF47: return ppu->bgp;
        case 0xFF48: return ppu->obp0;
        case 0xFF49: return ppu->obp1;
        case 0xFF4A: return ppu->wy;
        case 0xFF4B: return ppu->wx;
        default: return 0xFF;
    }
}

void gb_ppu_write_reg(gb_ppu_t *ppu, u16 addr, u8 value) {
    switch (addr) {
        case 0xFF40: ppu->lcdc = value; break;
        case 0xFF41: ppu->stat = (ppu->stat & 0x07) | (value & 0xF8); break;
        case 0xFF42: ppu->scy = value; break;
        case 0xFF43: ppu->scx = value; break;
        case 0xFF44: break; /* LY is read-only */
        case 0xFF45: ppu->lyc = value; break;
        case 0xFF47: ppu->bgp = value; break;
        case 0xFF48: ppu->obp0 = value; break;
        case 0xFF49: ppu->obp1 = value; break;
        case 0xFF4A: ppu->wy = value; break;
        case 0xFF4B: ppu->wx = value; break;
    }
}
