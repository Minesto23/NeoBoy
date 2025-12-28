#include "core.h"
#include "apu.h"
#include <string.h>

static const u8 pulse_duty_patterns[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1}, /* 12.5% */
    {1, 0, 0, 0, 0, 0, 0, 1}, /* 25% */
    {1, 0, 0, 0, 0, 1, 1, 1}, /* 50% */
    {0, 1, 1, 1, 1, 1, 1, 0}  /* 75% */
};

void gb_apu_init(gb_apu_t *apu) {
    if (!apu) return;
    memset(apu, 0, sizeof(gb_apu_t));
    apu->sample_rate = 44100;
    gb_apu_reset(apu);
}

void gb_apu_reset(gb_apu_t *apu) {
    if (!apu) return;
    u32 rate = apu->sample_rate;
    memset(apu, 0, sizeof(gb_apu_t));
    apu->sample_rate = rate;
    apu->nr52 = 0xF1; /* APU on by default on real hardware after boot */
    apu->ch4.lfsr = 0x7FFF;
}

static void step_frame_sequencer(gb_apu_t *apu) {
    apu->sequencer_step = (apu->sequencer_step + 1) % 8;
    
    /* Length Counters (256Hz: steps 0, 2, 4, 6) */
    if (apu->sequencer_step % 2 == 0) {
        if (apu->ch1.length_enabled && apu->ch1.length > 0) apu->ch1.length--;
        if (apu->ch2.length_enabled && apu->ch2.length > 0) apu->ch2.length--;
        if (apu->ch3.length_enabled && apu->ch3.length > 0) apu->ch3.length--;
        if (apu->ch4.length_enabled && apu->ch4.length > 0) apu->ch4.length--;
        
        if (apu->ch1.length == 0) apu->ch1.enabled = false;
        if (apu->ch2.length == 0) apu->ch2.enabled = false;
        if (apu->ch3.length == 0) apu->ch3.enabled = false;
        if (apu->ch4.length == 0) apu->ch4.enabled = false;
    }
    
    /* Sweep (128Hz: steps 2, 6) */
    if (apu->sequencer_step == 2 || apu->sequencer_step == 6) {
        if (apu->ch1.sweep_enabled && apu->ch1.sweep_period > 0) {
            apu->ch1.sweep_timer--;
            if (apu->ch1.sweep_timer == 0) {
                apu->ch1.sweep_timer = apu->ch1.sweep_period;
                /* Note: Real implementation needs frequency recalculation logic here */
            }
        }
    }
    
    /* Envelopes (64Hz: step 7) */
    if (apu->sequencer_step == 7) {
        if (apu->ch1.env_period > 0) {
            apu->ch1.env_timer--;
            if (apu->ch1.env_timer == 0) {
                apu->ch1.env_timer = apu->ch1.env_period;
                if (apu->ch1.env_direction && apu->ch1.env_volume < 15) apu->ch1.env_volume++;
                else if (!apu->ch1.env_direction && apu->ch1.env_volume > 0) apu->ch1.env_volume--;
            }
        }
        if (apu->ch2.env_period > 0) {
            apu->ch2.env_timer--;
            if (apu->ch2.env_timer == 0) {
                apu->ch2.env_timer = apu->ch2.env_period;
                if (apu->ch2.env_direction && apu->ch2.env_volume < 15) apu->ch2.env_volume++;
                else if (!apu->ch2.env_direction && apu->ch2.env_volume > 0) apu->ch2.env_volume--;
            }
        }
        if (apu->ch4.env_period > 0) {
            apu->ch4.env_timer--;
            if (apu->ch4.env_timer == 0) {
                apu->ch4.env_timer = apu->ch4.env_period;
                if (apu->ch4.env_direction && apu->ch4.env_volume < 15) apu->ch4.env_volume++;
                else if (!apu->ch4.env_direction && apu->ch4.env_volume > 0) apu->ch4.env_volume--;
            }
        }
    }
}

void gb_apu_step(gb_apu_t *apu, uint32_t cycles) {
    if (!(apu->nr52 & 0x80)) return; /* APU disabled */

    /* Frame Sequencer (512Hz) */
    apu->sequencer_timer += cycles;
    if (apu->sequencer_timer >= 8192) { /* 4.194304 MHz / 512 Hz = 8192 */
        apu->sequencer_timer -= 8192;
        step_frame_sequencer(apu);
    }

    /* Channel 4: Noise Oscillator */
    if (apu->ch4.enabled) {
        if (apu->ch4.timer <= cycles) {
            static const u8 divisors[] = {8, 16, 32, 48, 64, 80, 96, 112};
            apu->ch4.timer = divisors[apu->ch4.dividing_ratio] << apu->ch4.shift_clock_freq;
            
            u8 result = (apu->ch4.lfsr & 1) ^ ((apu->ch4.lfsr >> 1) & 1);
            apu->ch4.lfsr = (apu->ch4.lfsr >> 1) | (result << 14);
            if (apu->ch4.counter_step) {
                apu->ch4.lfsr = (apu->ch4.lfsr & ~0x40) | (result << 6);
            }
        } else {
            apu->ch4.timer -= cycles;
        }
    }

    /* Mixing and Sample Generation (Simplified) */
    static u32 sample_accumulation = 0;
    sample_accumulation += cycles;
    
    u32 cycles_per_sample = 4194304 / apu->sample_rate;
    
    if (sample_accumulation >= cycles_per_sample) {
        sample_accumulation -= cycles_per_sample;
        
        float ch1_sample = 0, ch2_sample = 0, ch3_sample = 0, ch4_sample = 0;
        
        if (apu->ch1.enabled) {
            ch1_sample = pulse_duty_patterns[apu->ch1.duty][apu->ch1.duty_step] ? 1.0f : -1.0f;
            ch1_sample *= (float)apu->ch1.env_volume / 15.0f;
        }
        
        if (apu->ch2.enabled) {
            ch2_sample = pulse_duty_patterns[apu->ch2.duty][apu->ch2.duty_step] ? 1.0f : -1.0f;
            ch2_sample *= (float)apu->ch2.env_volume / 15.0f;
        }
        
        if (apu->ch3.enabled) {
            u8 sample_idx = apu->ch3.sample_index % 32;
            u8 sample = apu->wave_ram[sample_idx / 2];
            if (sample_idx % 2 == 0) sample >>= 4;
            else sample &= 0x0F;
            
            if (apu->ch3.volume_shift > 0) sample >>= (apu->ch3.volume_shift - 1);
            else sample = 0;
            
            ch3_sample = ((float)sample / 7.5f) - 1.0f;
        }
        
        if (apu->ch4.enabled) {
            ch4_sample = (apu->ch4.lfsr & 1) ? -1.0f : 1.0f;
            ch4_sample *= (float)apu->ch4.env_volume / 15.0f;
        }
        
        float mixed = (ch1_sample + ch2_sample + ch3_sample + ch4_sample) * 0.25f;
        
        /* Store in circular buffer */
        if (apu->buffer_pos < 4096) {
            apu->buffer[apu->buffer_pos] = mixed;
            apu->buffer_pos = (apu->buffer_pos + 1) % 4096;
        } else {
            apu->buffer_pos = 0;
            apu->buffer[0] = mixed;
        }
    }
}

u8 gb_apu_read(gb_apu_t *apu, u16 addr) {
    if (addr >= 0xFF30 && addr < 0xFF40) {
        return apu->wave_ram[addr - 0xFF30];
    }

    switch (addr) {
        /* NR50-NR52 */
        case 0xFF24: return apu->nr50;
        case 0xFF25: return apu->nr51;
        case 0xFF26: return apu->nr52 | 0x70; /* Bits 4-6 always 1 */
        
        /* NR10-NR14 */
        case 0xFF10: return 0x80; /* CH1 Sweep (not fully implemented, return dummy) */
        case 0xFF11: return 0x3F | (apu->ch1.duty << 6);
        case 0xFF12: return 0x00; /* CH1 Envelope placeholder */
        case 0xFF14: return 0xBF | (apu->ch1.length_enabled << 6);
        
        /* CH2 */
        case 0xFF16: return 0x3F | (apu->ch2.duty << 6);
        case 0xFF19: return 0xBF | (apu->ch2.length_enabled << 6);
        
        /* CH3 */
        case 0xFF1A: return 0x7F | (apu->ch3.enabled << 7);
        case 0xFF1C: return 0x9F; /* CH3 Volume placeholder */
        case 0xFF1E: return 0xBF | (apu->ch3.length_enabled << 6);
        
        /* CH4 */
        case 0xFF23: return 0xBF | (apu->ch4.length_enabled << 6);
        
        default: return 0xFF;
    }
}

void gb_apu_write(gb_apu_t *apu, u16 addr, u8 value) {
    if (!(apu->nr52 & 0x80) && addr != 0xFF26 && addr < 0xFF30) {
        return; /* Registers locked if APU is off, except NR52 and Wave RAM */
    }

    if (addr >= 0xFF30 && addr < 0xFF40) {
        apu->wave_ram[addr - 0xFF30] = value;
        return;
    }

    switch (addr) {
        case 0xFF24: apu->nr50 = value; break;
        case 0xFF25: apu->nr51 = value; break;
        case 0xFF26: 
            if (!(value & 0x80)) {
                gb_apu_reset(apu);
                apu->nr52 = 0x00;
            } else if (!(apu->nr52 & 0x80)) {
                apu->nr52 |= 0x80;
                apu->sequencer_step = 0;
            }
            break;

        /* CH1: Pulse with sweep */
        case 0xFF10:
            apu->ch1.sweep_period = (value >> 4) & 0x07;
            apu->ch1.sweep_direction = value & 0x08;
            apu->ch1.sweep_shift = value & 0x07;
            break;
        case 0xFF11:
            apu->ch1.duty = value >> 6;
            apu->ch1.length = 64 - (value & 0x3F);
            break;
        case 0xFF12:
            apu->ch1.volume = value >> 4;
            apu->ch1.env_direction = value & 0x08;
            apu->ch1.env_period = value & 0x07;
            break;
        case 0xFF13:
            apu->ch1.frequency = (apu->ch1.frequency & 0x0700) | value;
            break;
        case 0xFF14:
            apu->ch1.frequency = (apu->ch1.frequency & 0xFF) | ((value & 0x07) << 8);
            apu->ch1.length_enabled = value & 0x40;
            if (value & 0x80) { /* Trigger */
                apu->ch1.enabled = true;
                if (apu->ch1.length == 0) apu->ch1.length = 64;
                apu->ch1.timer = (2048 - apu->ch1.frequency) * 4;
                apu->ch1.env_volume = apu->ch1.volume;
                apu->ch1.env_timer = apu->ch1.env_period;
            }
            break;

        /* CH2: Pulse */
        case 0xFF16:
            apu->ch2.duty = value >> 6;
            apu->ch2.length = 64 - (value & 0x3F);
            break;
        case 0xFF17:
            apu->ch2.volume = value >> 4;
            apu->ch2.env_direction = value & 0x08;
            apu->ch2.env_period = value & 0x07;
            break;
        case 0xFF18:
            apu->ch2.frequency = (apu->ch2.frequency & 0x0700) | value;
            break;
        case 0xFF19:
            apu->ch2.frequency = (apu->ch2.frequency & 0xFF) | ((value & 0x07) << 8);
            apu->ch2.length_enabled = value & 0x40;
            if (value & 0x80) { /* Trigger */
                apu->ch2.enabled = true;
                if (apu->ch2.length == 0) apu->ch2.length = 64;
                apu->ch2.timer = (2048 - apu->ch2.frequency) * 4;
                apu->ch2.env_volume = apu->ch2.volume;
                apu->ch2.env_timer = apu->ch2.env_period;
            }
            break;

        /* CH3: Wave */
        case 0xFF1A: apu->ch3.enabled = (value & 0x80); break;
        case 0xFF1B: apu->ch3.length = 256 - value; break;
        case 0xFF1C: apu->ch3.volume_shift = (value >> 5) & 0x03; break;
        case 0xFF1D: apu->ch3.frequency = (apu->ch3.frequency & 0x0700) | value; break;
        case 0xFF1E:
            apu->ch3.frequency = (apu->ch3.frequency & 0xFF) | ((value & 0x07) << 8);
            apu->ch3.length_enabled = value & 0x40;
            if (value & 0x80) {
                apu->ch3.enabled = true;
                if (apu->ch3.length == 0) apu->ch3.length = 256;
                apu->ch3.timer = (2048 - apu->ch3.frequency) * 2;
                apu->ch3.sample_index = 0;
            }
            break;

        /* CH4: Noise */
        case 0xFF20: apu->ch4.length = 64 - (value & 0x3F); break;
        case 0xFF21:
            apu->ch4.volume = value >> 4;
            apu->ch4.env_direction = value & 0x08;
            apu->ch4.env_period = value & 0x07;
            break;
        case 0xFF22:
            apu->ch4.shift_clock_freq = (value >> 4);
            apu->ch4.counter_step = (value & 0x08);
            apu->ch4.dividing_ratio = (value & 0x07);
            break;
        case 0xFF23:
            apu->ch4.length_enabled = value & 0x40;
            if (value & 0x80) {
                apu->ch4.enabled = true;
                if (apu->ch4.length == 0) apu->ch4.length = 64;
                apu->ch4.env_volume = apu->ch4.volume;
                apu->ch4.env_timer = apu->ch4.env_period;
                apu->ch4.lfsr = 0x7FFF;
            }
            break;
    }
}
