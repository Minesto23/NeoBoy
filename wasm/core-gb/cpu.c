/**
 * NeoBoy - Game Boy CPU (LR35902) Implementation
 * 
 * Purpose: CPU instruction execution and state management
 * 
 * This is a placeholder implementation that provides the basic structure
 * for CPU emulation. A full implementation would include:
 * - Complete instruction decode table (256 opcodes + 256 CB-prefixed)
 * - ALU operations (ADD, SUB, AND, OR, XOR, CP, INC, DEC)
 * - Load/Store operations
 * - Jump/Call/Return operations
 * - Bit operations (via CB prefix)
 * - Interrupt handling (VBLANK, LCD STAT, Timer, Serial, Joypad)
 * 
 * TODO: Implement full instruction set and timing
 */

#include "cpu.h"
#include "mmu.h"
#include <string.h>
#include <stdio.h>

void gb_cpu_init(gb_cpu_t *cpu) {
    memset(cpu, 0, sizeof(gb_cpu_t));
    
    /* Set power-on register values (from Pan Docs) */
    cpu->a = 0x01;
    cpu->f = 0xB0;
    cpu->b = 0x00;
    cpu->c = 0x13;
    cpu->d = 0x00;
    cpu->e = 0xD8;
    cpu->h = 0x01;
    cpu->l = 0x4D;
    cpu->sp = 0xFFFE;
    cpu->pc = 0x0100;  /* Start after boot ROM */
    
    cpu->ime = false;
    cpu->halted = false;
    cpu->stopped = false;
    cpu->cycles = 0;
}

void gb_cpu_reset(gb_cpu_t *cpu) {
    gb_cpu_init(cpu);
}

u32 gb_cpu_step(gb_cpu_t *cpu, void *mmu) {
    /* Handle halted state */
    if (cpu->halted) {
        return 4;  /* HALT consumes 4 cycles per iteration */
    }
    
    /* Fetch instruction */
    if (cpu->pc == 0x0100) {
        printf("CPU starting execution at 0x100...\n");
    }
    u8 opcode = gb_mmu_read(mmu, cpu->pc++);
    u32 cycles = 4;  /* Base cycle count */
    
    /* Instruction decoder */
    switch (opcode) {
        case 0x00: // NOP
            cycles = 4;
            break;

        case 0x3E: // LD A, n
            cpu->a = gb_mmu_read(mmu, cpu->pc++);
            cycles = 8;
            break;

        case 0xC3: // JP nn
        {
            u16 low = gb_mmu_read(mmu, cpu->pc++);
            u16 high = gb_mmu_read(mmu, cpu->pc++);
            cpu->pc = (high << 8) | low;
            cycles = 16;
            break;
        }

        case 0xAF: // XOR A
            cpu->a = 0;
            cpu->f = 0x80; // Zero flag
            cycles = 4;
            break;

        case 0x21: // LD HL, nn
        {
            cpu->l = gb_mmu_read(mmu, cpu->pc++);
            cpu->h = gb_mmu_read(mmu, cpu->pc++);
            cycles = 12;
            break;
        }

        default:
            /* For now, just NOP unknown opcodes but log them (if we had logging) */
            cycles = 4;
            break;
    }
    
    cpu->cycles += cycles;
    return cycles;
}

void gb_cpu_handle_interrupts(gb_cpu_t *cpu, void *mmu) {
    /*
     * PLACEHOLDER: Interrupt handling
     * 
     * Steps:
     * 1. Check if IME (Interrupt Master Enable) is set
     * 2. Read IE (0xFFFF) and IF (0xFF0F) registers
     * 3. Check for enabled and pending interrupts (IE & IF)
     * 4. Priority: VBlank > LCD STAT > Timer > Serial > Joypad
     * 5. If interrupt found:
     *    - Clear IME
     *    - Clear IF bit for the interrupt
     *    - PUSH PC to stack
     *    - Jump to interrupt vector
     */
    
    if (!cpu->ime) {
        return;
    }
    
    u8 ie = gb_mmu_read(mmu, 0xFFFF);
    u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
    u8 triggered = ie & if_reg;
    
    if (triggered == 0) {
        return;
    }
    
    /* Wake from HALT even if IME is disabled */
    cpu->halted = false;
    
    /* TODO: Implement interrupt service routine dispatch */
}
