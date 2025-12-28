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

typedef struct gb_ppu_t gb_ppu_t;
typedef struct gb_cartridge_t gb_cartridge_t;

typedef struct gb_mmu_t {
    /* Work RAM */
    u8 wram[0x2000];  /* 8KB Work RAM */
    u8 hram[0x7F];    /* 127 bytes High RAM */
    
    /* I/O Registers array */
    u8 io[0x80];
    
    /* Component references */
    gb_ppu_t *ppu;
    gb_cartridge_t *cart;
    
    /* Joypad state */
    u8 joypad;
} gb_mmu_t;

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
#define IO_IE    0xFFFF  /* Interrupt enable */

/* Function prototypes */

void gb_mmu_init(gb_mmu_t *mmu, gb_ppu_t *ppu, gb_cartridge_t *cart);
void gb_mmu_reset(gb_mmu_t *mmu);

u8 gb_mmu_read(gb_mmu_t *mmu, u16 addr);
void gb_mmu_write(gb_mmu_t *mmu, u16 addr, u8 value);

u16 gb_mmu_read16(gb_mmu_t *mmu, u16 addr);
void gb_mmu_write16(gb_mmu_t *mmu, u16 addr, u16 value);

#endif /* GB_MMU_H */
