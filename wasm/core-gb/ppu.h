/**
 * NeoBoy - Game Boy PPU (Picture Processing Unit) Header
 * 
 * Purpose: Graphics rendering for 160x144 monochrome display
 * 
 * The Game Boy PPU operates in several modes:
 * - Mode 0: H-Blank (204 cycles)
 * - Mode 1: V-Blank (4560 cycles)
 * - Mode 2: OAM Scan (80 cycles)
 * - Mode 3: Drawing (172 cycles)
 * 
 * Display specifications:
 * - Resolution: 160x144 pixels
 * - Color depth: 2-bit (4 shades of gray)
 * - Tiles: 8x8 pixels, 384 tiles in VRAM
 * - Background: 32x32 tile map (256x256 pixels)
 * - Window: Overlay layer
 * - Sprites (OBJ): 40 sprites, 10 per scanline, 8x8 or 8x16
 * 
 * TODO: Implement tile fetcher, sprite rendering, and scanline renderer
 */

#ifndef GB_PPU_H
#define GB_PPU_H

#include "../common/common.h"

#define GB_SCREEN_WIDTH  160
#define GB_SCREEN_HEIGHT 144
#define GB_FRAMEBUFFER_SIZE (GB_SCREEN_WIDTH * GB_SCREEN_HEIGHT * 4)  /* RGBA */

/* PPU modes */
typedef enum {
    PPU_MODE_HBLANK = 0,
    PPU_MODE_VBLANK = 1,
    PPU_MODE_OAM_SCAN = 2,
    PPU_MODE_DRAWING = 3
} gb_ppu_mode_t;

/* PPU state */
typedef struct gb_ppu_t {
    u8 vram[0x2000];      /* 8KB Video RAM */
    u8 oam[0xA0];         /* 160 bytes Object Attribute Memory (40 sprites) */
    
    /* LCD Control Register (0xFF40) */
    u8 lcdc;
    
    /* LCD Status Register (0xFF41) */
    u8 stat;
    
    /* Scroll registers */
    u8 scy;  /* Scroll Y (0xFF42) */
    u8 scx;  /* Scroll X (0xFF43) */
    
    /* LCD Y Coordinate (0xFF44) */
    u8 ly;
    
    /* LY Compare (0xFF45) */
    u8 lyc;
    
    /* Palette registers */
    u8 bgp;   /* Background Palette (0xFF47) */
    u8 obp0;  /* Object Palette 0 (0xFF48) */
    u8 obp1;  /* Object Palette 1 (0xFF49) */
    
    /* Window position */
    u8 wy;    /* Window Y (0xFF4A) */
    u8 wx;    /* Window X (0xFF4B) */
    
    /* Internal state */
    gb_ppu_mode_t mode;
    u32 mode_cycles;
    
    /* Framebuffer (RGBA format for easy rendering) */
    u8 framebuffer[GB_FRAMEBUFFER_SIZE];
} gb_ppu_t;

/* LCDC flags */
#define LCDC_ENABLE        (1 << 7)
#define LCDC_WIN_TILEMAP   (1 << 6)
#define LCDC_WIN_ENABLE    (1 << 5)
#define LCDC_BG_WIN_TILES  (1 << 4)
#define LCDC_BG_TILEMAP    (1 << 3)
#define LCDC_OBJ_SIZE      (1 << 2)
#define LCDC_OBJ_ENABLE    (1 << 1)
#define LCDC_BG_ENABLE     (1 << 0)

/* Function prototypes */

/**
 * Initialize PPU to power-on state
 */
void gb_ppu_init(gb_ppu_t *ppu);

/**
 * Reset PPU state
 */
void gb_ppu_reset(gb_ppu_t *ppu);

/**
 * Step PPU by given number of cycles
 * Returns true if a frame was completed
 */
bool gb_ppu_step(gb_ppu_t *ppu, u32 cycles);

/**
 * Render a scanline
 */
void gb_ppu_render_scanline(gb_ppu_t *ppu);

/**
 * Write to VRAM
 */
void gb_ppu_write_vram(gb_ppu_t *ppu, u16 addr, u8 value);

/**
 * Read from VRAM
 */
u8 gb_ppu_read_vram(gb_ppu_t *ppu, u16 addr);

/**
 * Write to OAM
 */
void gb_ppu_write_oam(gb_ppu_t *ppu, u16 addr, u8 value);

/**
 * Read from OAM
 */
u8 gb_ppu_read_oam(gb_ppu_t *ppu, u16 addr);

#endif /* GB_PPU_H */
