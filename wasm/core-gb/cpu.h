/**
 * NeoBoy - Game Boy CPU (LR35902) Header
 * 
 * Purpose: CPU emulation for the Sharp LR35902 processor
 * 
 * The LR35902 is a hybrid between the Intel 8080 and Zilog Z80:
 * - 8-bit data bus, 16-bit address bus
 * - 8-bit registers: A, F, B, C, D, E, H, L
 * - 16-bit register pairs: AF, BC, DE, HL
 * - 16-bit stack pointer (SP) and program counter (PC)
 * - Flag register (F): Z (Zero), N (Subtract), H (Half-Carry), C (Carry)
 * 
 * This file defines the CPU state structure and function prototypes
 * for instruction execution, register manipulation, and CPU stepping.
 */

#ifndef GB_CPU_H
#define GB_CPU_H

#include "../common/common.h"

/* CPU Registers */
typedef struct gb_cpu_t {
    /* 8-bit registers */
    u8 a, f;  /* Accumulator and Flags */
    u8 b, c;
    u8 d, e;
    u8 h, l;
    
    /* 16-bit registers */
    u16 sp;   /* Stack Pointer */
    u16 pc;   /* Program Counter */
    
    /* CPU state */
    bool ime;     /* Interrupt Master Enable */
    bool ei_delay; /* EI delay flag (enables IME after next instruction) */
    bool halted;  /* CPU halted state */
    bool stopped; /* CPU stopped state */
    
    u64 cycles;   /* Total cycles executed */
} gb_cpu_t;

/* Flag register bit positions */
#define FLAG_Z 7  /* Zero flag */
#define FLAG_N 6  /* Subtract flag */
#define FLAG_H 5  /* Half-carry flag */
#define FLAG_C 4  /* Carry flag */

/* Flag manipulation macros */
#define CPU_GET_FLAG(cpu, flag) (((cpu)->f >> (flag)) & 1)
#define CPU_SET_FLAG(cpu, flag) ((cpu)->f |= (1 << (flag)))
#define CPU_CLEAR_FLAG(cpu, flag) ((cpu)->f &= ~(1 << (flag)))

/* 16-bit register pair access */
#define CPU_GET_AF(cpu) (((u16)(cpu)->a << 8) | (cpu)->f)
#define CPU_GET_BC(cpu) (((u16)(cpu)->b << 8) | (cpu)->c)
#define CPU_GET_DE(cpu) (((u16)(cpu)->d << 8) | (cpu)->e)
#define CPU_GET_HL(cpu) (((u16)(cpu)->h << 8) | (cpu)->l)

#define CPU_SET_AF(cpu, val) do { (cpu)->a = (val) >> 8; (cpu)->f = (val) & 0xF0; } while(0)
#define CPU_SET_BC(cpu, val) do { (cpu)->b = (val) >> 8; (cpu)->c = (val) & 0xFF; } while(0)
#define CPU_SET_DE(cpu, val) do { (cpu)->d = (val) >> 8; (cpu)->e = (val) & 0xFF; } while(0)
#define CPU_SET_HL(cpu, val) do { (cpu)->h = (val) >> 8; (cpu)->l = (val) & 0xFF; } while(0)

/* Function prototypes */

/**
 * Initialize CPU to power-on state
 * Sets initial register values and clears flags
 */
void gb_cpu_init(gb_cpu_t *cpu);

/**
 * Reset CPU to known state
 * Similar to init but may preserve some state for soft reset
 */
void gb_cpu_reset(gb_cpu_t *cpu);

/**
 * Execute one CPU instruction
 * Returns the number of cycles taken
 * 
 * @param cpu CPU state
 * @param mmu Memory management unit for memory access
 * @return Number of machine cycles executed
 */
u32 gb_cpu_step(gb_cpu_t *cpu, void *mmu);

/**
 * Handle interrupts
 * Checks interrupt flags and executes interrupt service routine if needed
 */
u32 gb_cpu_handle_interrupts(gb_cpu_t *cpu, void *mmu);

#endif /* GB_CPU_H */
