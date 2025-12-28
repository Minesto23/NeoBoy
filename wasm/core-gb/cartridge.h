/**
 * NeoBoy - Game Boy Cartridge/MBC Header
 * 
 * Purpose: ROM loading and Memory Bank Controller (MBC) emulation
 * 
 * Supported MBCs:
 * - ROM ONLY (No MBC)
 * - MBC1 (max 2MB ROM, 32KB RAM)
 * - MBC3 (with RTC support)
 * - MBC5 (max 8MB ROM, 128KB RAM)
 * 
 * Cartridge header format (at 0x0100-0x014F):
 * - 0x0134-0x0143: Title
 * - 0x0147: Cartridge type (MBC indicator)
 * - 0x0148: ROM size
 * - 0x0149: RAM size
 */

#ifndef GB_CARTRIDGE_H
#define GB_CARTRIDGE_H

#include "../common/common.h"

#define MAX_ROM_SIZE (8 * 1024 * 1024)  /* 8MB */
#define MAX_RAM_SIZE (128 * 1024)       /* 128KB */

typedef enum {
    MBC_NONE = 0,
    MBC1,
    MBC2,
    MBC3,
    MBC5
} mbc_type_t;

typedef struct gb_cartridge_t {
    u8 *rom;              /* ROM data */
    u32 rom_size;         /* ROM size in bytes */
    u8 *ram;              /* External RAM */
    u32 ram_size;         /* RAM size in bytes */
    
    mbc_type_t mbc_type;  /* Memory Bank Controller type */
    
    /* MBC state */
    u16 rom_bank;         /* Current ROM bank */
    u8 ram_bank;          /* Current RAM bank */
    bool ram_enable;      /* RAM enable flag */
    
    /* MBC1 specific */
    u8 banking_mode;      /* 0: ROM banking, 1: RAM banking */
    
    /* MBC3 RTC specific */
    u8 rtc_regs[5];       /* RTC registers: S, M, H, DL, DH */
    u8 rtc_latch[5];      /* Latched RTC values */
    bool rtc_latched;     /* RTC latch flag */
    u64 rtc_base_time;    /* Base time for RTC emulation */

    /* MBC5 specific */
    u16 rom_bank_9bit;    /* MBC5 uses 9 bits for ROM banking */

    /* Cartridge info */
    char title[17];       /* Game title (null-terminated) */
} gb_cartridge_t;

/* Function prototypes */

/**
 * Initialize cartridge structure
 */
void gb_cart_init(gb_cartridge_t *cart);

/**
 * Load ROM from buffer
 * Parses cartridge header and sets up MBC
 */
int gb_cart_load(gb_cartridge_t *cart, const u8 *data, u32 size);

/**
 * Read from cartridge ROM
 */
u8 gb_cart_read(gb_cartridge_t *cart, u16 addr);

/**
 * Write to cartridge (MBC registers)
 */
void gb_cart_write(gb_cartridge_t *cart, u16 addr, u8 value);

/**
 * Read from external RAM
 */
u8 gb_cart_read_ram(gb_cartridge_t *cart, u16 addr);

/**
 * Write to external RAM
 */
void gb_cart_write_ram(gb_cartridge_t *cart, u16 addr, u8 value);

/**
 * Free cartridge resources
 */
void gb_cart_destroy(gb_cartridge_t *cart);

#endif /* GB_CARTRIDGE_H */
