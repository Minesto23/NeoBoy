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
    cpu->ei_delay = false;
    cpu->halted = false;
    cpu->stopped = false;
    cpu->halt_bug = false;
    cpu->cycles = 0;
}

void gb_cpu_reset(gb_cpu_t *cpu) {
    gb_cpu_init(cpu);
}

static u8 fetch_u8(gb_cpu_t *cpu, gb_mmu_t *mmu) {
    return gb_mmu_read(mmu, cpu->pc++);
}

static u16 fetch_u16(gb_cpu_t *cpu, gb_mmu_t *mmu) {
    u16 val = gb_mmu_read16(mmu, cpu->pc);
    cpu->pc += 2;
    return val;
}

static void push16(gb_cpu_t *cpu, gb_mmu_t *mmu, u16 val) {
    cpu->sp -= 2;
    gb_mmu_write16(mmu, cpu->sp, val);
}

static u16 pop16(gb_cpu_t *cpu, gb_mmu_t *mmu) {
    u16 val = gb_mmu_read16(mmu, cpu->sp);
    cpu->sp += 2;
    return val;
}

/* ALU Helpers */

static void alu_add(gb_cpu_t *cpu, u8 val) {
    u16 res = (u16)cpu->a + val;
    cpu->f = 0;
    if ((res & 0xFF) == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if ((cpu->a & 0xF) + (val & 0xF) > 0xF) CPU_SET_FLAG(cpu, FLAG_H);
    if (res > 0xFF) CPU_SET_FLAG(cpu, FLAG_C);
    cpu->a = (u8)res;
}

static void alu_adc(gb_cpu_t *cpu, u8 val) {
    u8 carry = CPU_GET_FLAG(cpu, FLAG_C);
    u32 res = (u32)cpu->a + val + carry;
    cpu->f = 0;
    if ((res & 0xFF) == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if ((cpu->a & 0xF) + (val & 0xF) + carry > 0xF) CPU_SET_FLAG(cpu, FLAG_H);
    if (res > 0xFF) CPU_SET_FLAG(cpu, FLAG_C);
    cpu->a = (u8)res;
}

static void alu_sub(gb_cpu_t *cpu, u8 val) {
    cpu->f = (1 << FLAG_N);
    if (cpu->a == val) CPU_SET_FLAG(cpu, FLAG_Z);
    if ((cpu->a & 0xF) < (val & 0xF)) CPU_SET_FLAG(cpu, FLAG_H);
    if (cpu->a < val) CPU_SET_FLAG(cpu, FLAG_C);
    cpu->a -= val;
}

static void alu_sbc(gb_cpu_t *cpu, u8 val) {
    u8 carry = CPU_GET_FLAG(cpu, FLAG_C);
    int res = (int)cpu->a - val - carry;
    cpu->f = (1 << FLAG_N);
    if ((res & 0xFF) == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if ((int)(cpu->a & 0xF) - (int)(val & 0xF) - (int)carry < 0) CPU_SET_FLAG(cpu, FLAG_H);
    if (res < 0) CPU_SET_FLAG(cpu, FLAG_C);
    cpu->a = (u8)res;
}

static void alu_and(gb_cpu_t *cpu, u8 val) {
    cpu->a &= val;
    cpu->f = (1 << FLAG_H); // H is set for AND
    if (cpu->a == 0) CPU_SET_FLAG(cpu, FLAG_Z);
}

static void alu_or(gb_cpu_t *cpu, u8 val) {
    cpu->a |= val;
    cpu->f = 0;
    if (cpu->a == 0) CPU_SET_FLAG(cpu, FLAG_Z);
}

static void alu_xor(gb_cpu_t *cpu, u8 val) {
    cpu->a ^= val;
    cpu->f = 0;
    if (cpu->a == 0) CPU_SET_FLAG(cpu, FLAG_Z);
}

static void alu_cp(gb_cpu_t *cpu, u8 val) {
    u8 temp_a = cpu->a;
    alu_sub(cpu, val);
    cpu->a = temp_a; // CP is like SUB but doesn't affect A
}

static void alu_inc(gb_cpu_t *cpu, u8 *reg) {
    u8 res = *reg + 1;
    CPU_CLEAR_FLAG(cpu, FLAG_Z);
    CPU_CLEAR_FLAG(cpu, FLAG_N);
    CPU_CLEAR_FLAG(cpu, FLAG_H);
    if (res == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if ((*reg & 0xF) == 0xF) CPU_SET_FLAG(cpu, FLAG_H);
    *reg = res;
}

static void alu_dec(gb_cpu_t *cpu, u8 *reg) {
    u8 res = *reg - 1;
    CPU_CLEAR_FLAG(cpu, FLAG_Z);
    CPU_SET_FLAG(cpu, FLAG_N);
    CPU_CLEAR_FLAG(cpu, FLAG_H);
    if (res == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if ((*reg & 0xF) == 0) CPU_SET_FLAG(cpu, FLAG_H);
    *reg = res;
}

/* CB Helpers */

static void cb_rlc(gb_cpu_t *cpu, u8 *reg) {
    u8 carry = (*reg & 0x80) >> 7;
    *reg = (*reg << 1) | carry;
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_rrc(gb_cpu_t *cpu, u8 *reg) {
    u8 carry = *reg & 0x01;
    *reg = (*reg >> 1) | (carry << 7);
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_rl(gb_cpu_t *cpu, u8 *reg) {
    u8 old_carry = CPU_GET_FLAG(cpu, FLAG_C);
    u8 new_carry = (*reg & 0x80) >> 7;
    *reg = (*reg << 1) | old_carry;
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (new_carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_rr(gb_cpu_t *cpu, u8 *reg) {
    u8 old_carry = CPU_GET_FLAG(cpu, FLAG_C);
    u8 new_carry = *reg & 0x01;
    *reg = (*reg >> 1) | (old_carry << 7);
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (new_carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_sla(gb_cpu_t *cpu, u8 *reg) {
    u8 carry = (*reg & 0x80) >> 7;
    *reg <<= 1;
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_sra(gb_cpu_t *cpu, u8 *reg) {
    u8 carry = *reg & 0x01;
    *reg = (u8)((s8)*reg >> 1);
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_swap(gb_cpu_t *cpu, u8 *reg) {
    *reg = ((*reg & 0x0F) << 4) | ((*reg & 0xF0) >> 4);
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
}

static void cb_srl(gb_cpu_t *cpu, u8 *reg) {
    u8 carry = *reg & 0x01;
    *reg >>= 1;
    cpu->f = 0;
    if (*reg == 0) CPU_SET_FLAG(cpu, FLAG_Z);
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cb_bit(gb_cpu_t *cpu, u8 bit, u8 val) {
    CPU_CLEAR_FLAG(cpu, FLAG_N);
    CPU_SET_FLAG(cpu, FLAG_H);
    if (!(val & (1 << bit))) CPU_SET_FLAG(cpu, FLAG_Z);
    else CPU_CLEAR_FLAG(cpu, FLAG_Z);
}

/* Standard Rotations (Accumulator) */

static void cpu_rlca(gb_cpu_t *cpu) {
    u8 carry = (cpu->a & 0x80) >> 7;
    cpu->a = (cpu->a << 1) | carry;
    cpu->f = 0;
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
    // Z flag is ALWAYS cleared for RLCA/RRCA/RLA/RRA
}

static void cpu_rrca(gb_cpu_t *cpu) {
    u8 carry = cpu->a & 0x01;
    cpu->a = (cpu->a >> 1) | (carry << 7);
    cpu->f = 0;
    if (carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cpu_rla(gb_cpu_t *cpu) {
    u8 old_carry = CPU_GET_FLAG(cpu, FLAG_C);
    u8 new_carry = (cpu->a & 0x80) >> 7;
    cpu->a = (cpu->a << 1) | old_carry;
    cpu->f = 0;
    if (new_carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static void cpu_rra(gb_cpu_t *cpu) {
    u8 old_carry = CPU_GET_FLAG(cpu, FLAG_C);
    u8 new_carry = cpu->a & 0x01;
    cpu->a = (cpu->a >> 1) | (old_carry << 7);
    cpu->f = 0;
    if (new_carry) CPU_SET_FLAG(cpu, FLAG_C);
}

static u32 gb_cpu_execute_cb(gb_cpu_t *cpu, gb_mmu_t *mmu) {
    u8 opcode = fetch_u8(cpu, mmu);
    u8 *reg = NULL;
    u16 hl = 0;
    u8 hl_val = 0;
    bool is_hl = false;

    /* Identify target register/memory */
    switch (opcode & 0x07) {
        case 0: reg = &cpu->b; break;
        case 1: reg = &cpu->c; break;
        case 2: reg = &cpu->d; break;
        case 3: reg = &cpu->e; break;
        case 4: reg = &cpu->h; break;
        case 5: reg = &cpu->l; break;
        case 6:
            hl = CPU_GET_HL(cpu);
            hl_val = gb_mmu_read(mmu, hl);
            reg = &hl_val;
            is_hl = true;
            break;
        case 7: reg = &cpu->a; break;
    }

    if (opcode < 0x08) cb_rlc(cpu, reg);
    else if (opcode < 0x10) cb_rrc(cpu, reg);
    else if (opcode < 0x18) cb_rl(cpu, reg);
    else if (opcode < 0x20) cb_rr(cpu, reg);
    else if (opcode < 0x28) cb_sla(cpu, reg);
    else if (opcode < 0x30) cb_sra(cpu, reg);
    else if (opcode < 0x38) cb_swap(cpu, reg);
    else if (opcode < 0x40) cb_srl(cpu, reg);
    else if (opcode < 0x80) cb_bit(cpu, (opcode >> 3) & 0x07, *reg);
    else if (opcode < 0xC0) *reg &= ~(1 << ((opcode >> 3) & 0x07)); // RES
    else *reg |= (1 << ((opcode >> 3) & 0x07)); // SET

    if (is_hl) {
        gb_mmu_write(mmu, hl, hl_val);
    }
    
    /* Cycle correction */
    if (is_hl) {
        // BIT (HL) is 12 cycles (4 fetch + 8 exec)
        // Others (HL) are 16 cycles (4 fetch + 12 exec) 
        // gb_cpu_step adds 4 base cycles.
        if ((opcode & 0xC0) == 0x40) return 8; // BIT (HL) -> Total 12
        return 12; // Others (HL) -> Total 16
    }

    // Register CB ops are 8 cycles total (4 fetch + 4 exec)
    return 4; 
}

u32 gb_cpu_step(gb_cpu_t *cpu, void *mmu_ptr) {
    gb_mmu_t *mmu = (gb_mmu_t *)mmu_ptr;
    
    /* Handle suspended states */
    if (cpu->stopped) {
        // STOP state is exited by a joypad interrupt (high-to-low transition on P1 bits)
        // For now, we'll implement a simple check: if any button is pressed (joypad interrupt pending), wake up.
        // In reality, it doesn't even need the interrupt enabled in IE, just the signal.
        u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
        if (if_reg & 0x10) { // Joypad interrupt bit
             cpu->stopped = false;
        } else {
             return 4; // Burn cycles while stopped
        }
    }

    /* Handle halted state */
    if (cpu->halted) {
        u8 ie = gb_mmu_read(mmu, 0xFFFF);
        u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
        if (ie & if_reg) {
            cpu->halted = false;
        }
        return 4;
    }
    
    /* Handle EI delay */
    if (cpu->ei_delay) {
        cpu->ime = true;
        cpu->ei_delay = false;
    }
    
    /* Fetch instruction */
    u16 old_pc = cpu->pc;
    u8 opcode = fetch_u8(cpu, mmu);
    
    /* Halt bug:
     * If halt_bug triggered, the PC fails to increment for one instruction fetch.
     * Effectively, we re-execute the byte at the current PC.
     */
    if (cpu->halt_bug) {
        cpu->pc = old_pc;
        cpu->halt_bug = false;
    }

    u32 cycles = 4;
    
    /* Instruction decoder */
    switch (opcode) {
        case 0x00: // NOP
            cycles = 4;
            break;

        /* --- 8-bit Loads --- */
        
        // LD r, n
        case 0x06: cpu->b = fetch_u8(cpu, mmu); cycles = 8; break; // LD B, n
        case 0x0E: cpu->c = fetch_u8(cpu, mmu); cycles = 8; break; // LD C, n
        case 0x16: cpu->d = fetch_u8(cpu, mmu); cycles = 8; break; // LD D, n
        case 0x1E: cpu->e = fetch_u8(cpu, mmu); cycles = 8; break; // LD E, n
        case 0x26: cpu->h = fetch_u8(cpu, mmu); cycles = 8; break; // LD H, n
        case 0x2E: cpu->l = fetch_u8(cpu, mmu); cycles = 8; break; // LD L, n
        case 0x3E: cpu->a = fetch_u8(cpu, mmu); cycles = 8; break; // LD A, n
        case 0x36: gb_mmu_write(mmu, CPU_GET_HL(cpu), fetch_u8(cpu, mmu)); cycles = 12; break; // LD (HL), n

        // LD r, r
        case 0x7F: cpu->a = cpu->a; cycles = 4; break; // LD A, A
        case 0x78: cpu->a = cpu->b; cycles = 4; break; // LD A, B
        case 0x79: cpu->a = cpu->c; cycles = 4; break; // LD A, C
        case 0x7A: cpu->a = cpu->d; cycles = 4; break; // LD A, D
        case 0x7B: cpu->a = cpu->e; cycles = 4; break; // LD A, E
        case 0x7C: cpu->a = cpu->h; cycles = 4; break; // LD A, H
        case 0x7D: cpu->a = cpu->l; cycles = 4; break; // LD A, L
        case 0x7E: cpu->a = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD A, (HL)

        case 0x40: cpu->b = cpu->b; cycles = 4; break; // LD B, B
        case 0x41: cpu->b = cpu->c; cycles = 4; break; // LD B, C
        case 0x42: cpu->b = cpu->d; cycles = 4; break; // LD B, D
        case 0x43: cpu->b = cpu->e; cycles = 4; break; // LD B, E
        case 0x44: cpu->b = cpu->h; cycles = 4; break; // LD B, H
        case 0x45: cpu->b = cpu->l; cycles = 4; break; // LD B, L
        case 0x46: cpu->b = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD B, (HL)
        case 0x47: cpu->b = cpu->a; cycles = 4; break; // LD B, A

        case 0x48: cpu->c = cpu->b; cycles = 4; break; // LD C, B
        case 0x49: cpu->c = cpu->c; cycles = 4; break; // LD C, C
        case 0x4A: cpu->c = cpu->d; cycles = 4; break; // LD C, D
        case 0x4B: cpu->c = cpu->e; cycles = 4; break; // LD C, E
        case 0x4C: cpu->c = cpu->h; cycles = 4; break; // LD C, H
        case 0x4D: cpu->c = cpu->l; cycles = 4; break; // LD C, L
        case 0x4E: cpu->c = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD C, (HL)
        case 0x4F: cpu->c = cpu->a; cycles = 4; break; // LD C, A

        case 0x50: cpu->d = cpu->b; cycles = 4; break; // LD D, B
        case 0x51: cpu->d = cpu->c; cycles = 4; break; // LD D, C
        case 0x52: cpu->d = cpu->d; cycles = 4; break; // LD D, D
        case 0x53: cpu->d = cpu->e; cycles = 4; break; // LD D, E
        case 0x54: cpu->d = cpu->h; cycles = 4; break; // LD D, H
        case 0x55: cpu->d = cpu->l; cycles = 4; break; // LD D, L
        case 0x56: cpu->d = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD D, (HL)
        case 0x57: cpu->d = cpu->a; cycles = 4; break; // LD D, A

        case 0x58: cpu->e = cpu->b; cycles = 4; break; // LD E, B
        case 0x59: cpu->e = cpu->c; cycles = 4; break; // LD E, C
        case 0x5A: cpu->e = cpu->d; cycles = 4; break; // LD E, D
        case 0x5B: cpu->e = cpu->e; cycles = 4; break; // LD E, E
        case 0x5C: cpu->e = cpu->h; cycles = 4; break; // LD E, H
        case 0x5D: cpu->e = cpu->l; cycles = 4; break; // LD E, L
        case 0x5E: cpu->e = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD E, (HL)
        case 0x5F: cpu->e = cpu->a; cycles = 4; break; // LD E, A

        case 0x60: cpu->h = cpu->b; cycles = 4; break; // LD H, B
        case 0x61: cpu->h = cpu->c; cycles = 4; break; // LD H, C
        case 0x62: cpu->h = cpu->d; cycles = 4; break; // LD H, D
        case 0x63: cpu->h = cpu->e; cycles = 4; break; // LD H, E
        case 0x64: cpu->h = cpu->h; cycles = 4; break; // LD H, H
        case 0x65: cpu->h = cpu->l; cycles = 4; break; // LD H, L
        case 0x66: cpu->h = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD H, (HL)
        case 0x67: cpu->h = cpu->a; cycles = 4; break; // LD H, A

        case 0x68: cpu->l = cpu->b; cycles = 4; break; // LD L, B
        case 0x69: cpu->l = cpu->c; cycles = 4; break; // LD L, C
        case 0x6A: cpu->l = cpu->d; cycles = 4; break; // LD L, D
        case 0x6B: cpu->l = cpu->e; cycles = 4; break; // LD L, E
        case 0x6C: cpu->l = cpu->h; cycles = 4; break; // LD L, H
        case 0x6D: cpu->l = cpu->l; cycles = 4; break; // LD L, L
        case 0x6E: cpu->l = gb_mmu_read(mmu, CPU_GET_HL(cpu)); cycles = 8; break; // LD L, (HL)
        case 0x6F: cpu->l = cpu->a; cycles = 4; break; // LD L, A

        case 0x70: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->b); cycles = 8; break; // LD (HL), B
        case 0x71: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->c); cycles = 8; break; // LD (HL), C
        case 0x72: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->d); cycles = 8; break; // LD (HL), D
        case 0x73: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->e); cycles = 8; break; // LD (HL), E
        case 0x74: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->h); cycles = 8; break; // LD (HL), H
        case 0x75: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->l); cycles = 8; break; // LD (HL), L
        case 0x77: gb_mmu_write(mmu, CPU_GET_HL(cpu), cpu->a); cycles = 8; break; // LD (HL), A

        // LD A, (nn) / (nn), A
        case 0xFA: cpu->a = gb_mmu_read(mmu, fetch_u16(cpu, mmu)); cycles = 16; break; // LD A, (nn)
        case 0xEA: gb_mmu_write(mmu, fetch_u16(cpu, mmu), cpu->a); cycles = 16; break; // LD (nn), A

        // LDH (FF00+n), A / LDH A, (FF00+n)
        case 0xE0: gb_mmu_write(mmu, 0xFF00 + fetch_u8(cpu, mmu), cpu->a); cycles = 12; break; // LDH (n), A
        case 0xF0: cpu->a = gb_mmu_read(mmu, 0xFF00 + fetch_u8(cpu, mmu)); cycles = 12; break; // LDH A, (n)

        // LD (BC), A / LD (DE), A / LD A, (BC) / LD A, (DE)
        case 0x02: gb_mmu_write(mmu, CPU_GET_BC(cpu), cpu->a); cycles = 8; break;
        case 0x12: gb_mmu_write(mmu, CPU_GET_DE(cpu), cpu->a); cycles = 8; break;
        case 0x0A: cpu->a = gb_mmu_read(mmu, CPU_GET_BC(cpu)); cycles = 8; break;
        case 0x1A: cpu->a = gb_mmu_read(mmu, CPU_GET_DE(cpu)); cycles = 8; break;

        // LD (HL+), A / LD (HL-), A / LD A, (HL+) / LD A, (HL-)
        case 0x22: 
        {
            u16 hl = CPU_GET_HL(cpu);
            gb_mmu_write(mmu, hl++, cpu->a);
            CPU_SET_HL(cpu, hl);
            cycles = 8;
            break;
        }
        case 0x32:
        {
            u16 hl = CPU_GET_HL(cpu);
            gb_mmu_write(mmu, hl--, cpu->a);
            CPU_SET_HL(cpu, hl);
            cycles = 8;
            break;
        }
        case 0x2A:
        {
            u16 hl = CPU_GET_HL(cpu);
            cpu->a = gb_mmu_read(mmu, hl++);
            CPU_SET_HL(cpu, hl);
            cycles = 8;
            break;
        }
        case 0x3A:
        {
            u16 hl = CPU_GET_HL(cpu);
            cpu->a = gb_mmu_read(mmu, hl--);
            CPU_SET_HL(cpu, hl);
            cycles = 8;
            break;
        }

        /* --- 16-bit Loads --- */

        case 0x01: CPU_SET_BC(cpu, fetch_u16(cpu, mmu)); cycles = 12; break; // LD BC, nn
        case 0x11: CPU_SET_DE(cpu, fetch_u16(cpu, mmu)); cycles = 12; break; // LD DE, nn
        case 0x21: CPU_SET_HL(cpu, fetch_u16(cpu, mmu)); cycles = 12; break; // LD HL, nn
        case 0x31: cpu->sp = fetch_u16(cpu, mmu); cycles = 12; break;         // LD SP, nn
        case 0x08: gb_mmu_write16(mmu, fetch_u16(cpu, mmu), cpu->sp); cycles = 20; break; // LD (nn), SP

        case 0xF9: cpu->sp = CPU_GET_HL(cpu); cycles = 8; break; // LD SP, HL

        /* --- 8-bit Arithmetic/Logic --- */

        // ADD A, r
        case 0x87: alu_add(cpu, cpu->a); cycles = 4; break;
        case 0x80: alu_add(cpu, cpu->b); cycles = 4; break;
        case 0x81: alu_add(cpu, cpu->c); cycles = 4; break;
        case 0x82: alu_add(cpu, cpu->d); cycles = 4; break;
        case 0x83: alu_add(cpu, cpu->e); cycles = 4; break;
        case 0x84: alu_add(cpu, cpu->h); cycles = 4; break;
        case 0x85: alu_add(cpu, cpu->l); cycles = 4; break;
        case 0x86: alu_add(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xC6: alu_add(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // ADD A, n

        // ADC A, r
        case 0x8F: alu_adc(cpu, cpu->a); cycles = 4; break;
        case 0x88: alu_adc(cpu, cpu->b); cycles = 4; break;
        case 0x89: alu_adc(cpu, cpu->c); cycles = 4; break;
        case 0x8A: alu_adc(cpu, cpu->d); cycles = 4; break;
        case 0x8B: alu_adc(cpu, cpu->e); cycles = 4; break;
        case 0x8C: alu_adc(cpu, cpu->h); cycles = 4; break;
        case 0x8D: alu_adc(cpu, cpu->l); cycles = 4; break;
        case 0x8E: alu_adc(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xCE: alu_adc(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // ADC A, n

        // SUB r
        case 0x97: alu_sub(cpu, cpu->a); cycles = 4; break;
        case 0x90: alu_sub(cpu, cpu->b); cycles = 4; break;
        case 0x91: alu_sub(cpu, cpu->c); cycles = 4; break;
        case 0x92: alu_sub(cpu, cpu->d); cycles = 4; break;
        case 0x93: alu_sub(cpu, cpu->e); cycles = 4; break;
        case 0x94: alu_sub(cpu, cpu->h); cycles = 4; break;
        case 0x95: alu_sub(cpu, cpu->l); cycles = 4; break;
        case 0x96: alu_sub(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xD6: alu_sub(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // SUB n

        // SBC A, r
        case 0x9F: alu_sbc(cpu, cpu->a); cycles = 4; break;
        case 0x98: alu_sbc(cpu, cpu->b); cycles = 4; break;
        case 0x99: alu_sbc(cpu, cpu->c); cycles = 4; break;
        case 0x9A: alu_sbc(cpu, cpu->d); cycles = 4; break;
        case 0x9B: alu_sbc(cpu, cpu->e); cycles = 4; break;
        case 0x9C: alu_sbc(cpu, cpu->h); cycles = 4; break;
        case 0x9D: alu_sbc(cpu, cpu->l); cycles = 4; break;
        case 0x9E: alu_sbc(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xDE: alu_sbc(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // SBC A, n

        // AND r
        case 0xA7: alu_and(cpu, cpu->a); cycles = 4; break;
        case 0xA0: alu_and(cpu, cpu->b); cycles = 4; break;
        case 0xA1: alu_and(cpu, cpu->c); cycles = 4; break;
        case 0xA2: alu_and(cpu, cpu->d); cycles = 4; break;
        case 0xA3: alu_and(cpu, cpu->e); cycles = 4; break;
        case 0xA4: alu_and(cpu, cpu->h); cycles = 4; break;
        case 0xA5: alu_and(cpu, cpu->l); cycles = 4; break;
        case 0xA6: alu_and(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xE6: alu_and(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // AND n

        // XOR r
        case 0xAF: alu_xor(cpu, cpu->a); cycles = 4; break;
        case 0xA8: alu_xor(cpu, cpu->b); cycles = 4; break;
        case 0xA9: alu_xor(cpu, cpu->c); cycles = 4; break;
        case 0xAA: alu_xor(cpu, cpu->d); cycles = 4; break;
        case 0xAB: alu_xor(cpu, cpu->e); cycles = 4; break;
        case 0xAC: alu_xor(cpu, cpu->h); cycles = 4; break;
        case 0xAD: alu_xor(cpu, cpu->l); cycles = 4; break;
        case 0xAE: alu_xor(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xEE: alu_xor(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // XOR n

        // OR r
        case 0xB7: alu_or(cpu, cpu->a); cycles = 4; break;
        case 0xB0: alu_or(cpu, cpu->b); cycles = 4; break;
        case 0xB1: alu_or(cpu, cpu->c); cycles = 4; break;
        case 0xB2: alu_or(cpu, cpu->d); cycles = 4; break;
        case 0xB3: alu_or(cpu, cpu->e); cycles = 4; break;
        case 0xB4: alu_or(cpu, cpu->h); cycles = 4; break;
        case 0xB5: alu_or(cpu, cpu->l); cycles = 4; break;
        case 0xB6: alu_or(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xF6: alu_or(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // OR n

        // CP r
        case 0xBF: alu_cp(cpu, cpu->a); cycles = 4; break;
        case 0xB8: alu_cp(cpu, cpu->b); cycles = 4; break;
        case 0xB9: alu_cp(cpu, cpu->c); cycles = 4; break;
        case 0xBA: alu_cp(cpu, cpu->d); cycles = 4; break;
        case 0xBB: alu_cp(cpu, cpu->e); cycles = 4; break;
        case 0xBC: alu_cp(cpu, cpu->h); cycles = 4; break;
        case 0xBD: alu_cp(cpu, cpu->l); cycles = 4; break;
        case 0xBE: alu_cp(cpu, gb_mmu_read(mmu, CPU_GET_HL(cpu))); cycles = 8; break;
        case 0xFE: alu_cp(cpu, fetch_u8(cpu, mmu)); cycles = 8; break; // CP n

        // INC r
        case 0x3C: alu_inc(cpu, &cpu->a); cycles = 4; break;
        case 0x04: alu_inc(cpu, &cpu->b); cycles = 4; break;
        case 0x0C: alu_inc(cpu, &cpu->c); cycles = 4; break;
        case 0x14: alu_inc(cpu, &cpu->d); cycles = 4; break;
        case 0x1C: alu_inc(cpu, &cpu->e); cycles = 4; break;
        case 0x24: alu_inc(cpu, &cpu->h); cycles = 4; break;
        case 0x2C: alu_inc(cpu, &cpu->l); cycles = 4; break;
        case 0x34: 
        {
            u16 hl = CPU_GET_HL(cpu);
            u8 val = gb_mmu_read(mmu, hl);
            alu_inc(cpu, &val);
            gb_mmu_write(mmu, hl, val);
            cycles = 12;
            break;
        }

        // DEC r
        case 0x3D: alu_dec(cpu, &cpu->a); cycles = 4; break;
        case 0x05: alu_dec(cpu, &cpu->b); cycles = 4; break;
        case 0x0D: alu_dec(cpu, &cpu->c); cycles = 4; break;
        case 0x15: alu_dec(cpu, &cpu->d); cycles = 4; break;
        case 0x1D: alu_dec(cpu, &cpu->e); cycles = 4; break;
        case 0x25: alu_dec(cpu, &cpu->h); cycles = 4; break;
        case 0x2D: alu_dec(cpu, &cpu->l); cycles = 4; break;
        case 0x35:
        {
            u16 hl = CPU_GET_HL(cpu);
            u8 val = gb_mmu_read(mmu, hl);
            alu_dec(cpu, &val);
            gb_mmu_write(mmu, hl, val);
            cycles = 12;
            break;
        }

        /* --- 16-bit Arithmetic --- */

        // INC rr
        case 0x03: CPU_SET_BC(cpu, CPU_GET_BC(cpu) + 1); cycles = 8; break;
        case 0x13: CPU_SET_DE(cpu, CPU_GET_DE(cpu) + 1); cycles = 8; break;
        case 0x23: CPU_SET_HL(cpu, CPU_GET_HL(cpu) + 1); cycles = 8; break;
        case 0x33: cpu->sp++; cycles = 8; break;

        // DEC rr
        case 0x0B: CPU_SET_BC(cpu, CPU_GET_BC(cpu) - 1); cycles = 8; break;
        case 0x1B: CPU_SET_DE(cpu, CPU_GET_DE(cpu) - 1); cycles = 8; break;
        case 0x2B: CPU_SET_HL(cpu, CPU_GET_HL(cpu) - 1); cycles = 8; break;
        case 0x3B: cpu->sp--; cycles = 8; break;

        // ADD HL, rr
        case 0x09: 
        {
            u16 hl = CPU_GET_HL(cpu);
            u16 rr = CPU_GET_BC(cpu);
            CPU_CLEAR_FLAG(cpu, FLAG_N);
            if ((hl & 0x0FFF) + (rr & 0x0FFF) > 0x0FFF) CPU_SET_FLAG(cpu, FLAG_H);
            if ((u32)hl + rr > 0xFFFF) CPU_SET_FLAG(cpu, FLAG_C);
            CPU_SET_HL(cpu, hl + rr);
            cycles = 8;
            break;
        }
        case 0x19:
        {
            u16 hl = CPU_GET_HL(cpu);
            u16 rr = CPU_GET_DE(cpu);
            CPU_CLEAR_FLAG(cpu, FLAG_N);
            if ((hl & 0x0FFF) + (rr & 0x0FFF) > 0x0FFF) CPU_SET_FLAG(cpu, FLAG_H);
            if ((u32)hl + rr > 0xFFFF) CPU_SET_FLAG(cpu, FLAG_C);
            CPU_SET_HL(cpu, hl + rr);
            cycles = 8;
            break;
        }
        case 0x29:
        {
            u16 hl = CPU_GET_HL(cpu);
            u16 rr = CPU_GET_HL(cpu);
            CPU_CLEAR_FLAG(cpu, FLAG_N);
            if ((hl & 0x0FFF) + (rr & 0x0FFF) > 0x0FFF) CPU_SET_FLAG(cpu, FLAG_H);
            if ((u32)hl + rr > 0xFFFF) CPU_SET_FLAG(cpu, FLAG_C);
            CPU_SET_HL(cpu, hl + rr);
            cycles = 8;
            break;
        }
        case 0x39:
        {
            u16 hl = CPU_GET_HL(cpu);
            u16 rr = cpu->sp;
            CPU_CLEAR_FLAG(cpu, FLAG_N);
            if ((hl & 0x0FFF) + (rr & 0x0FFF) > 0x0FFF) CPU_SET_FLAG(cpu, FLAG_H);
            if ((u32)hl + rr > 0xFFFF) CPU_SET_FLAG(cpu, FLAG_C);
            CPU_SET_HL(cpu, hl + rr);
            cycles = 8;
            break;
        }

        case 0xE8: // ADD SP, n
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            cpu->f = 0;
            if ((cpu->sp & 0xF) + (rel & 0xF) > 0xF) CPU_SET_FLAG(cpu, FLAG_H);
            if ((cpu->sp & 0xFF) + (rel & 0xFF) > 0xFF) CPU_SET_FLAG(cpu, FLAG_C);
            cpu->sp += rel;
            cycles = 16;
            break;
        }

        case 0xF8: // LD HL, SP+n
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            cpu->f = 0;
            if ((cpu->sp & 0xF) + (rel & 0xF) > 0xF) CPU_SET_FLAG(cpu, FLAG_H);
            if ((cpu->sp & 0xFF) + (rel & 0xFF) > 0xFF) CPU_SET_FLAG(cpu, FLAG_C);
            CPU_SET_HL(cpu, cpu->sp + rel);
            cycles = 12;
            break;
        }

        /* --- Stack Operations --- */

        case 0xC5: push16(cpu, mmu, CPU_GET_BC(cpu)); cycles = 16; break; // PUSH BC
        case 0xD5: push16(cpu, mmu, CPU_GET_DE(cpu)); cycles = 16; break; // PUSH DE
        case 0xE5: push16(cpu, mmu, CPU_GET_HL(cpu)); cycles = 16; break; // PUSH HL
        case 0xF5: push16(cpu, mmu, CPU_GET_AF(cpu)); cycles = 16; break; // PUSH AF

        case 0xC1: CPU_SET_BC(cpu, pop16(cpu, mmu)); cycles = 12; break; // POP BC
        case 0xD1: CPU_SET_DE(cpu, pop16(cpu, mmu)); cycles = 12; break; // POP DE
        case 0xE1: CPU_SET_HL(cpu, pop16(cpu, mmu)); cycles = 12; break; // POP HL
        case 0xF1: CPU_SET_AF(cpu, pop16(cpu, mmu)); cycles = 12; break; // POP AF

        /* --- Control Flow --- */

        case 0xC3: // JP nn
            cpu->pc = fetch_u16(cpu, mmu);
            cycles = 16;
            break;

        case 0xC2: if (!CPU_GET_FLAG(cpu, FLAG_Z)) { cpu->pc = fetch_u16(cpu, mmu); cycles = 16; } else { cpu->pc += 2; cycles = 12; } break; // JP NZ, nn
        case 0xCA: if (CPU_GET_FLAG(cpu, FLAG_Z)) { cpu->pc = fetch_u16(cpu, mmu); cycles = 16; } else { cpu->pc += 2; cycles = 12; } break; // JP Z, nn
        case 0xD2: if (!CPU_GET_FLAG(cpu, FLAG_C)) { cpu->pc = fetch_u16(cpu, mmu); cycles = 16; } else { cpu->pc += 2; cycles = 12; } break; // JP NC, nn
        case 0xDA: if (CPU_GET_FLAG(cpu, FLAG_C)) { cpu->pc = fetch_u16(cpu, mmu); cycles = 16; } else { cpu->pc += 2; cycles = 12; } break; // JP C, nn

        case 0xE9: cpu->pc = CPU_GET_HL(cpu); cycles = 4; break; // JP (HL)

        // JR n
        case 0x18: 
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            cpu->pc += rel;
            cycles = 12;
            break;
        }
        case 0x20: // JR NZ, n
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            if (!CPU_GET_FLAG(cpu, FLAG_Z)) { cpu->pc += rel; cycles = 12; } else { cycles = 8; }
            break;
        }
        case 0x28: // JR Z, n
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            if (CPU_GET_FLAG(cpu, FLAG_Z)) { cpu->pc += rel; cycles = 12; } else { cycles = 8; }
            break;
        }
        case 0x30: // JR NC, n
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            if (!CPU_GET_FLAG(cpu, FLAG_C)) { cpu->pc += rel; cycles = 12; } else { cycles = 8; }
            break;
        }
        case 0x38: // JR C, n
        {
            s8 rel = (s8)fetch_u8(cpu, mmu);
            if (CPU_GET_FLAG(cpu, FLAG_C)) { cpu->pc += rel; cycles = 12; } else { cycles = 8; }
            break;
        }

        // CALL nn
        case 0xCD:
        {
            u16 dest = fetch_u16(cpu, mmu);
            push16(cpu, mmu, cpu->pc);
            cpu->pc = dest;
            cycles = 24;
            break;
        }
        case 0xC4: // CALL NZ, nn
        {
            u16 dest = fetch_u16(cpu, mmu);
            if (!CPU_GET_FLAG(cpu, FLAG_Z)) { push16(cpu, mmu, cpu->pc); cpu->pc = dest; cycles = 24; } else { cycles = 12; }
            break;
        }
        case 0xCC: // CALL Z, nn
        {
            u16 dest = fetch_u16(cpu, mmu);
            if (CPU_GET_FLAG(cpu, FLAG_Z)) { push16(cpu, mmu, cpu->pc); cpu->pc = dest; cycles = 24; } else { cycles = 12; }
            break;
        }
        case 0xD4: // CALL NC, nn
        {
            u16 dest = fetch_u16(cpu, mmu);
            if (!CPU_GET_FLAG(cpu, FLAG_C)) { push16(cpu, mmu, cpu->pc); cpu->pc = dest; cycles = 24; } else { cycles = 12; }
            break;
        }
        case 0xDC: // CALL C, nn
        {
            u16 dest = fetch_u16(cpu, mmu);
            if (CPU_GET_FLAG(cpu, FLAG_C)) { push16(cpu, mmu, cpu->pc); cpu->pc = dest; cycles = 24; } else { cycles = 12; }
            break;
        }

        // RET
        case 0xC9: cpu->pc = pop16(cpu, mmu); cycles = 16; break;
        case 0xC0: if (!CPU_GET_FLAG(cpu, FLAG_Z)) { cpu->pc = pop16(cpu, mmu); cycles = 20; } else { cycles = 8; } break; // RET NZ
        case 0xC8: if (CPU_GET_FLAG(cpu, FLAG_Z)) { cpu->pc = pop16(cpu, mmu); cycles = 20; } else { cycles = 8; } break; // RET Z
        case 0xD0: if (!CPU_GET_FLAG(cpu, FLAG_C)) { cpu->pc = pop16(cpu, mmu); cycles = 20; } else { cycles = 8; } break; // RET NC
        case 0xD8: if (CPU_GET_FLAG(cpu, FLAG_C)) { cpu->pc = pop16(cpu, mmu); cycles = 20; } else { cycles = 8; } break; // RET C
        case 0xD9: cpu->pc = pop16(cpu, mmu); cpu->ime = true; cycles = 16; break; // RETI

        // RST
        case 0xC7: push16(cpu, mmu, cpu->pc); cpu->pc = 0x00; cycles = 16; break;
        case 0xCF: push16(cpu, mmu, cpu->pc); cpu->pc = 0x08; cycles = 16; break;
        case 0xD7: push16(cpu, mmu, cpu->pc); cpu->pc = 0x10; cycles = 16; break;
        case 0xDF: push16(cpu, mmu, cpu->pc); cpu->pc = 0x18; cycles = 16; break;
        case 0xE7: push16(cpu, mmu, cpu->pc); cpu->pc = 0x20; cycles = 16; break;
        case 0xEF: push16(cpu, mmu, cpu->pc); cpu->pc = 0x28; cycles = 16; break;
        case 0xF7: push16(cpu, mmu, cpu->pc); cpu->pc = 0x30; cycles = 16; break;
        case 0xFF: 
            printf("[CRASH DETECTED] Executing RST 38 (0xFF) at PC: 0x%04X\n", old_pc);
            push16(cpu, mmu, cpu->pc); cpu->pc = 0x38; cycles = 16; break;

        /* --- Miscellaneous --- */

        case 0x27: // DAA (Decimal Adjust Accumulator)
        {
            u8 correction = 0;
            if (CPU_GET_FLAG(cpu, FLAG_H) || (!CPU_GET_FLAG(cpu, FLAG_N) && (cpu->a & 0x0F) > 9)) {
                correction |= 0x06;
            }
            if (CPU_GET_FLAG(cpu, FLAG_C) || (!CPU_GET_FLAG(cpu, FLAG_N) && cpu->a > 0x99)) {
                correction |= 0x60;
                CPU_SET_FLAG(cpu, FLAG_C);
            }
            if (CPU_GET_FLAG(cpu, FLAG_N)) {
                cpu->a -= correction;
            } else {
                cpu->a += correction;
            }
            CPU_CLEAR_FLAG(cpu, FLAG_Z);
            CPU_CLEAR_FLAG(cpu, FLAG_H);
            if (cpu->a == 0) CPU_SET_FLAG(cpu, FLAG_Z);
            cycles = 4;
            break;
        }

        case 0x2F: cpu->a = ~cpu->a; CPU_SET_FLAG(cpu, FLAG_N); CPU_SET_FLAG(cpu, FLAG_H); cycles = 4; break; // CPL
        case 0x37: CPU_CLEAR_FLAG(cpu, FLAG_N); CPU_CLEAR_FLAG(cpu, FLAG_H); CPU_SET_FLAG(cpu, FLAG_C); cycles = 4; break; // SCF
        case 0x3F: CPU_CLEAR_FLAG(cpu, FLAG_N); CPU_CLEAR_FLAG(cpu, FLAG_H); if (CPU_GET_FLAG(cpu, FLAG_C)) CPU_CLEAR_FLAG(cpu, FLAG_C); else CPU_SET_FLAG(cpu, FLAG_C); cycles = 4; break; // CCF

        case 0x76: // HALT
        {
            u8 ie = gb_mmu_read(mmu, 0xFFFF);
            u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
            if (!cpu->ime && (ie & if_reg & 0x1F)) {
                // HALT bug: HALT mode not entered, next instruction executed twice
                cpu->halt_bug = true;
            } else {
                cpu->halted = true;
            }
            cycles = 4;
            break;
        }
        case 0x10: cpu->stopped = true; fetch_u8(cpu, mmu); cycles = 4; break; // STOP
        case 0xF3: cpu->ime = false; cpu->ei_delay = false; cycles = 4; break; // DI
        case 0xFB: cpu->ei_delay = true; cycles = 4; break; // EI

        // Rotates (Acc only)
        case 0x07: // RLCA
        {
            u8 carry = (cpu->a & 0x80) >> 7;
            cpu->a = (cpu->a << 1) | carry;
            cpu->f = 0;
            if (carry) CPU_SET_FLAG(cpu, FLAG_C);
            cycles = 4;
            break;
        }
        case 0x0F: // RRCA
        {
            u8 carry = cpu->a & 0x01;
            cpu->a = (cpu->a >> 1) | (carry << 7);
            cpu->f = 0;
            if (carry) CPU_SET_FLAG(cpu, FLAG_C);
            cycles = 4;
            break;
        }
        case 0x17: // RLA
        {
            u8 old_carry = CPU_GET_FLAG(cpu, FLAG_C);
            u8 new_carry = (cpu->a & 0x80) >> 7;
            cpu->a = (cpu->a << 1) | old_carry;
            cpu->f = 0;
            if (new_carry) CPU_SET_FLAG(cpu, FLAG_C);
            cycles = 4;
            break;
        }
        case 0x1F: // RRA
        {
            u8 old_carry = CPU_GET_FLAG(cpu, FLAG_C);
            u8 new_carry = cpu->a & 0x01;
            cpu->a = (cpu->a >> 1) | (old_carry << 7);
            cpu->f = 0;
            if (new_carry) CPU_SET_FLAG(cpu, FLAG_C);
            cycles = 4;
            break;
        }

        /* --- CB Prefix (Extended Instructions) --- */
        case 0xCB:
            cycles = 4 + gb_cpu_execute_cb(cpu, mmu);
            break;

        default:
            /* For now, just NOP unknown opcodes but log them (if we had logging) */
            cycles = 4;
            break;
    }
    
    cpu->cycles += cycles;
    return cycles;
}

u32 gb_cpu_handle_interrupts(gb_cpu_t *cpu, void *mmu_ptr) {
    gb_mmu_t *mmu = (gb_mmu_t *)mmu_ptr;
    
    u8 ie = gb_mmu_read(mmu, 0xFFFF);
    u8 if_reg = gb_mmu_read(mmu, 0xFF0F);
    u8 triggered = ie & if_reg;
    
    if (triggered == 0) {
        return 0;
    }
    
    /* Wake from HALT even if IME is disabled */
    cpu->halted = false;
    
    if (!cpu->ime) {
        return 0;
    }
    
    /* Priority: VBlank > LCD STAT > Timer > Serial > Joypad */
    u8 interrupt = 0;
    u16 vector = 0;
    
    if (triggered & 0x01) { interrupt = 0x01; vector = 0x40; }      /* VBlank */
    else if (triggered & 0x02) { interrupt = 0x02; vector = 0x48; } /* LCD STAT */
    else if (triggered & 0x04) { interrupt = 0x04; vector = 0x50; } /* Timer */
    else if (triggered & 0x08) { interrupt = 0x08; vector = 0x58; } /* Serial */
    else if (triggered & 0x10) { interrupt = 0x10; vector = 0x60; } /* Joypad */
    
    if (interrupt) {
        cpu->ime = false;
        gb_mmu_write(mmu, 0xFF0F, if_reg & ~interrupt);
        push16(cpu, mmu, cpu->pc);
        cpu->pc = vector;
        cpu->cycles += 20;
        return 20;
    }
    return 0;
}
