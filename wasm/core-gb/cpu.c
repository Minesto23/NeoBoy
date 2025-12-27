/**
 * NeoBoy - Game Boy CPU Implementation
 * 
 * Implements the Sharp LR35902 CPU (8-bit, Z80-like)
 * - 8-bit data bus, 16-bit address bus
 * - Registers: A, B, C, D, E, F, H, L, SP, PC
 * - Clock speed: 4.194304 MHz
 * 
 * TODO: Implement complete instruction set and timing
 */

#include "core.h"
#include <stdio.h>
#include <string.h>

// CPU Registers
typedef struct {
    uint8_t a, f;  // Accumulator and Flags
    uint8_t b, c;
    uint8_t d, e;
    uint8_t h, l;
    uint16_t sp;   // Stack Pointer
    uint16_t pc;   // Program Counter
    bool halted;
    bool ime;      // Interrupt Master Enable
} CPU;

static CPU cpu;

// Flag register bits
#define FLAG_Z 0x80  // Zero
#define FLAG_N 0x40  // Subtract
#define FLAG_H 0x20  // Half Carry
#define FLAG_C 0x10  // Carry

void cpu_init(void) {
    memset(&cpu, 0, sizeof(CPU));
    
    // Initial register values after boot ROM
    cpu.a = 0x01;
    cpu.f = 0xB0;
    cpu.b = 0x00;
    cpu.c = 0x13;
    cpu.d = 0x00;
    cpu.e = 0xD8;
    cpu.h = 0x01;
    cpu.l = 0x4D;
    cpu.sp = 0xFFFE;
    cpu.pc = 0x0100;  // Entry point after boot ROM
    cpu.ime = false;
}

void cpu_reset(void) {
    cpu_init();
}

/**
 * Execute a single CPU instruction
 * @return Number of cycles consumed
 */
int cpu_step(void) {
    if (cpu.halted) {
        return 4;  // HALT consumes 4 cycles
    }
    
    // TODO: Fetch opcode from memory
    // uint8_t opcode = mmu_read(cpu.pc++);
    
    // TODO: Decode and execute instruction
    // TODO: Handle interrupts if IME is set
    
    // Placeholder: NOP instruction (4 cycles)
    return 4;
}

uint16_t cpu_get_pc(void) {
    return cpu.pc;
}

void cpu_set_pc(uint16_t value) {
    cpu.pc = value;
}

// TODO: Implement full instruction set
// - Arithmetic and logic operations
// - Load/store instructions
// - Jump and call instructions
// - Bit operations
// - Rotate and shift operations
