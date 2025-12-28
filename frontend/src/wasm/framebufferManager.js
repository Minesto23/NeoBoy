/**
 * NeoBoy - Framebuffer Manager
 * 
 * Purpose: Efficient framebuffer transfer and rendering
 * 
 * Handles:
 * - Shared memory between WASM and Canvas
 * - Double buffering
 * - Format conversion (if needed)
 * - Performance optimizations
 */

/**
 * Framebuffer Manager Class
 */
export class FramebufferManager {
    constructor(width, height) {
        this.width = width;
        this.height = height;
        this.size = width * height * 4; // RGBA

        // Create ImageData for canvas rendering
        this.imageData = new ImageData(width, height);

        // Typed array view of the framebuffer
        this.buffer = null;
    }

    /**
     * Initialize framebuffer with WASM memory
     * @param {WebAssembly.Memory} wasmMemory - WASM memory object
     * @param {number} framebufferPtr - Pointer to framebuffer in WASM memory
     */
    init(wasmMemory, framebufferPtr) {
        // Create a view of WASM memory at the framebuffer location
        this.buffer = new Uint8ClampedArray(
            wasmMemory.buffer,
            framebufferPtr,
            this.size
        );
    }

    /**
     * Update ImageData from WASM framebuffer
     * This method copies data from WASM memory to Canvas-compatible format
     */
    updateImageData() {
        if (!this.buffer) {
            console.warn('Framebuffer not initialized');
            return;
        }

        // Direct copy - both are RGBA format
        this.imageData.data.set(this.buffer);
    }

    /**
     * Get ImageData for canvas rendering
     * @returns {ImageData} Canvas-compatible image data
     */
    getImageData() {
        return this.imageData;
    }

    /**
     * Clear framebuffer (fill with transparent black)
     */
    clear() {
        if (this.buffer) {
            this.buffer.fill(0);
        }
    }

    /**
     * Apply post-processing effects (optional)
     * @param {string} effect - Effect name ('scanlines', 'crt', etc.)
     */
    applyEffect(effect) {
        switch (effect) {
            case 'scanlines':
                this.applyScanlines();
                break;
            case 'crt':
                this.applyCRTEffect();
                break;
            default:
                // No effect
                break;
        }
    }

    /**
     * Apply scanline effect (simple darkening of alternating rows)
     */
    applyScanlines() {
        const data = this.imageData.data;
        for (let y = 0; y < this.height; y += 2) {
            for (let x = 0; x < this.width; x++) {
                const idx = (y * this.width + x) * 4;
                data[idx + 0] *= 0.7; // R
                data[idx + 1] *= 0.7; // G
                data[idx + 2] *= 0.7; // B
            }
        }
    }

    /**
     * Apply simple CRT effect (placeholder)
     */
    applyCRTEffect() {
        // PLACEHOLDER: More sophisticated CRT shader would go here
        this.applyScanlines();
    }
}

export default FramebufferManager;
