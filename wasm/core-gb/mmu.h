/**
 * NeoBoy - Game Boy MMU (Memory Management Unit) Header
 * 
 * Purpose: Memory mapping and access control for Game Boy address space
 * 
 * Memory Map:
 * 0x0000-0x3FFF: ROM Bank 00 (16KB)
 * 0x4000-0x7FFF: ROM Bank 01-NN (16KB switchable via MBC)
 * 0x8000-0x9FFF: VRAM (8KB)
 * 0xA000-0xBFFF: External RAM (8KB switchable via MBC)
 * 0xC000-0xCFFF: Work RAM Bank 0 (4KB)
 * 0xD000-0xDFFF: Work RAM Bank 1 (4KB)
 * 0xE000-0xFDFF: Echo RAM (mirror of C000-DDFF)
 * 0xFE00-0xFE9F: OAM (Object Attribute Memory)
 * 0xFEA0-0xFEFF: Unusable
 * 0xFF00-0xFF7F: I/O Registers
 * 0xFF80-0xFFFE: High RAM (HRAM)
 * 0xFFFF: Interrupt Enable Register
 */

#ifndef GB_MMU_H
#define GB_MMU_H

#include "../common/common.h"

struct gb_ppu_t;
struct gb_apu_t;
struct gb_cartridge_t;

typedef struct gb_mmu_t {
    /* Work RAM */
    u8 wram[0x8000];  /* 32KB Work RAM (8 banks of 4KB) */
    u8 hram[0x7F];    /* 127 bytes High RAM */
    
    /* I/O Registers array */
    u8 io[0x80];
    
    /* Component references */
    struct gb_ppu_t *ppu;
    struct gb_cartridge_t *cart;
    
    /* Joypad state */
    u8 joypad;

    /* Timer state */
    u16 div_counter;      /* Internal 16-bit counter for DIV */
    u32 tima_counter;     /* Internal counter for TIMA */
    
    /* GBC Control */
    u8 svbk;      /* WRAM Bank (0xFF70) */
    u8 key1;      /* Speed Switch (0xFF4D) */
    bool speed;   /* Current speed: 0=Normal, 1=Double */
    
    /* GBC HDMA */
    u8 hdma1, hdma2, hdma3, hdma4, hdma5;
    bool hdma_active;

    /* APU reference */
    struct gb_apu_t *apu;

    /* Interrupt Enable register (0xFFFF) */
    u8 ie;
} gb_mmu_t;

/* I/O Register addresses */
/* I/O Register addresses */
#define IO_JOYP  0xFF00  /* Joypad */
#define IO_SB    0xFF01  /* Serial transfer data */
#define IO_SC    0xFF02  /* Serial transfer control */
#define IO_DIV   0xFF04  /* Divider register */
#define IO_TIMA  0xFF05  /* Timer counter */
#define IO_TMA   0xFF06  /* Timer modulo */
#define IO_TAC   0xFF07  /* Timer control */
#define IO_IF    0xFF0F  /* Interrupt flag */
#define IO_LCDC  0xFF40  /* LCD control */
#define IO_STAT  0xFF41  /* LCD status */
#define IO_SCY   0xFF42  /* Scroll Y */
#define IO_SCX   0xFF43  /* Scroll X */
#define IO_LY    0xFF44  /* LCD Y coordinate */
#define IO_LYC   0xFF45  /* LY compare */
#define IO_DMA   0xFF46  /* DMA transfer */
#define IO_BGP   0xFF47  /* BG palette */
#define IO_OBP0  0xFF48  /* OBJ palette 0 */
#define IO_OBP1  0xFF49  /* OBJ palette 1 */
#define IO_WY    0xFF4A  /* Window Y */
#define IO_WX    0xFF4B  /* Window X */
#define IO_KEY1  0xFF4D  /* CGB Speed switch */
#define IO_VBK   0xFF4F  /* CGB VRAM bank */
#define IO_HDMA1 0xFF51  /* CGB HDMA1 */
#define IO_HDMA2 0xFF52  /* CGB HDMA2 */
#define IO_HDMA3 0xFF53  /* CGB HDMA3 */
#define IO_HDMA4 0xFF54  /* CGB HDMA4 */
#define IO_HDMA5 0xFF55  /* CGB HDMA5 */
#define IO_BCPS  0xFF68  /* CGB BG Palette Index */
#define IO_BCPD  0xFF69  /* CGB BG Palette Data */
#define IO_OCPS  0xFF6A  /* CGB OBJ Palette Index */
#define IO_OCPD  0xFF6B  /* CGB OBJ Palette Data */
#define IO_SVBK  0xFF70  /* CGB WRAM bank */
#define IO_IE    0xFFFF  /* Interrupt enable */

/* Function prototypes */
void gb_mmu_init(gb_mmu_t *mmu, struct gb_ppu_t *ppu, struct gb_apu_t *apu, struct gb_cartridge_t *cart);
void gb_mmu_reset(gb_mmu_t *mmu);

/**
 * Step timers by given number of cycles
 */
void gb_mmu_step_timers(gb_mmu_t *mmu, u32 cycles);

/**
 * Read a byte from memory
 */
u8 gb_mmu_read(gb_mmu_t *mmu, u16 addr);
void gb_mmu_write(gb_mmu_t *mmu, u16 addr, u8 value);

u16 gb_mmu_read16(gb_mmu_t *mmu, u16 addr);
void gb_mmu_write16(gb_mmu_t *mmu, u16 addr, u16 value);

/**
 * Execute one step (16 bytes) of HDMA transfer
 */
void gb_mmu_execute_hdma(gb_mmu_t *mmu);

#endif /* GB_MMU_H */
