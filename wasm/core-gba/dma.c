/**
 * NeoBoy - Game Boy Advance DMA Controller
 * 
 * 4 DMA channels for data transfer
 * - DMA0: Highest priority (internal use)
 * - DMA1-2: Sound FIFO
 * - DMA3: General purpose
 * 
 * Transfer modes:
 * - Immediate
 * - VBlank
 * - HBlank
 * - Special (sound FIFO)
 */

#include "core.h"
#include <string.h>

typedef struct {
    uint32_t src;
    uint32_t dst;
    uint16_t count;
    uint16_t control;
    bool enabled;
    bool repeat;
} DMAChannel;

static DMAChannel dma[4];

void dma_init(void) {
    memset(dma, 0, sizeof(dma));
}

void dma_reset(void) {
    dma_init();
}

/**
 * Trigger DMA transfer
 */
void dma_transfer(int channel) {
    if (!dma[channel].enabled) {
        return;
    }
    
    // TODO: Implement DMA transfer logic
    // - Read from source
    // - Write to destination
    // - Handle 16-bit or 32-bit transfers
    // - Increment/decrement/fixed source and destination
}

/**
 * Write to DMA control register
 */
void dma_write_control(int channel, uint16_t value) {
    dma[channel].control = value;
    dma[channel].enabled = (value & 0x8000) != 0;
    
    // TODO: Parse control bits
    // - Transfer size (16/32-bit)
    // - Timing mode
    // - Source/dest increment mode
    // - Repeat
    // - IRQ enable
}

// TODO: Implement DMA trigger on VBlank/HBlank
// TODO: Implement sound FIFO DMA
