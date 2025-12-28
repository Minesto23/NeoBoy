/**
 * NeoBoy - WASM Bindings
 * 
 * Provides a clean API for interacting with emulator cores,
 * supporting both raw WASM instances and Emscripten Module objects.
 */

export const GameBoyButton = {
    A: 0,
    B: 1,
    SELECT: 2,
    START: 3,
    RIGHT: 4,
    LEFT: 5,
    UP: 6,
    DOWN: 7
};

export class EmulatorCore {
    constructor(wasmModule, coreName) {
        this.wasm = wasmModule;
        this.coreName = coreName;
        this.initialized = false;
        this.bindFunctions();
    }

    bindFunctions() {
        const prefix = this.coreName === 'gb' ? 'gb_' :
            this.coreName === 'gbc' ? 'gbc_' : 'gba_';

        // Support both Emscripten (prefixed with _) and raw WASM (direct names)
        const getExport = (name) => {
            // Check direct property (Emscripten style)
            if (this.wasm[`_${name}`]) return this.wasm[`_${name}`];
            // Check exports object (Raw WASM style)
            if (this.wasm.exports && this.wasm.exports[name]) return this.wasm.exports[name];
            // Check with core prefix
            const prefixed = `${prefix}${name}`;
            if (this.wasm[`_${prefixed}`]) return this.wasm[`_${prefixed}`];
            if (this.wasm.exports && this.wasm.exports[prefixed]) return this.wasm.exports[prefixed];

            return null;
        };

        this.malloc = getExport('malloc');
        this.free = getExport('free');
        this.init = getExport('init');
        this.loadRom = getExport('load_rom');
        this.stepFrame = getExport('step_frame');
        this.setButton = getExport('set_button');
        this.getFramebuffer = getExport('get_framebuffer');
        this.getAudioBufferPtr = getExport('get_audio_buffer');
        this.getAudioBufferSize = getExport('get_audio_buffer_size');
        this.saveState = getExport('save_state');
        this.loadState = getExport('load_state');
        this.reset = getExport('reset');
        this.destroy = getExport('destroy');

        // Memory views
        this.updateMemoryViews();
    }

    updateMemoryViews() {
        // Emscripten provides HEAPU8, HEAPU32 etc. on the Module object
        if (this.wasm.HEAPU8) {
            this.HEAPU8 = this.wasm.HEAPU8;
        }
        // fallback to raw wasm exports if available
        else if (this.wasm.exports && this.wasm.exports.memory) {
            this.HEAPU8 = new Uint8Array(this.wasm.exports.memory.buffer);
        }
        // or check if it's on the wasm object directly (Emscripten sometimes does this)
        else if (this.wasm.wasmMemory) {
            this.HEAPU8 = new Uint8Array(this.wasm.wasmMemory.buffer);
        }
        else if (this.wasm.memory) {
            this.HEAPU8 = new Uint8Array(this.wasm.memory.buffer);
        }

        if (!this.HEAPU8) {
            console.error('Failed to locate WASM memory/HEAPU8');
        }
    }

    initialize() {
        if (this.init) {
            this.init();
            this.initialized = true;
        } else {
            console.warn('WASM core missing init function, proceeding as if initialized');
            this.initialized = true;
        }
    }

    load(romData) {
        if (!this.initialized) this.initialize();
        if (!this.malloc || !this.free || !this.loadRom) return false;

        const romPtr = this.malloc(romData.length);
        this.updateMemoryViews(); // Ensure views are fresh if memory grew during malloc
        this.HEAPU8.set(romData, romPtr);

        const result = this.loadRom(romPtr, romData.length);
        this.free(romPtr);

        return result === 0;
    }

    step() {
        if (this.stepFrame) this.stepFrame();
    }

    setButtonState(button, pressed) {
        if (this.setButton) this.setButton(button, pressed ? 1 : 0);
    }

    getFramebufferImageData(width, height) {
        if (!this.getFramebuffer) return null;

        const fbPtr = this.getFramebuffer();
        const fbSize = width * height * 4;

        this.updateMemoryViews();
        // Use Uint8ClampedArray for ImageData
        const memory = new Uint8ClampedArray(this.HEAPU8.buffer, fbPtr, fbSize);
        return new ImageData(memory, width, height);
    }

    getAudioSamples() {
        if (!this.getAudioBufferPtr || !this.getAudioBufferSize) return null;

        const ptr = this.getAudioBufferPtr();
        const size = this.getAudioBufferSize();

        this.updateMemoryViews();
        // Return a copy to avoid memory issues with WASM growth
        // Float32Array because samples are floats
        const samples = new Float32Array(this.wasm.HEAPF32.buffer, ptr, size);
        return new Float32Array(samples);
    }

    save() {
        if (!this.saveState || !this.malloc) return null;

        const bufferSize = 1024 * 1024; // 1MB allocation for save state
        const bufferPtr = this.malloc(bufferSize);
        const size = this.saveState(bufferPtr);

        this.updateMemoryViews();
        const saveData = this.HEAPU8.slice(bufferPtr, bufferPtr + size);
        this.free(bufferPtr);

        return saveData;
    }

    loadSave(saveData) {
        if (!this.loadState || !this.malloc) return false;

        const bufferPtr = this.malloc(saveData.length);
        this.updateMemoryViews(); // Ensure views are fresh if memory grew during malloc
        this.HEAPU8.set(saveData, bufferPtr);

        const result = this.loadState(bufferPtr, saveData.length);
        this.free(bufferPtr);

        return result === 0;
    }

    resetCore() {
        if (this.reset) this.reset();
    }

    cleanup() {
        if (this.destroy) this.destroy();
        this.initialized = false;
    }
}

export default EmulatorCore;
