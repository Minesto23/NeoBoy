/**
 * NeoBoy - WASM Module Loader
 * 
 * Purpose: Load and initialize WASM cores using Emscripten JS glue
 */

/**
 * Load a WASM core module
 * @param {string} coreName - Name of the core ('gb', 'gbc', or 'gba')
 * @returns {Promise<any>} The loaded Emscripten Module
 */
export async function loadWASMCore(coreName) {
    try {
        let loadModule;

        // Dynamic import of the generated JS glue
        // Vite handles these imports if they are relative to the current file
        switch (coreName) {
            case 'gb':
                loadModule = (await import('./generated/gb.js')).default;
                break;
            // case 'gbc':
            //     loadModule = (await import('./generated/gbc.js')).default;
            //     break;
            // case 'gba':
            //     loadModule = (await import('./generated/gba.js')).default;
            //     break;
            default:
                throw new Error(`Unknown core: ${coreName}`);
        }

        console.log(`Initializing Emscripten module for ${coreName}...`);

        // Emscripten factory function returns a Promise that resolves to the Module
        const wasmModule = await loadModule();

        return wasmModule;
    } catch (error) {
        console.error(`Error loading WASM core ${coreName}:`, error);
        throw error;
    }
}

/**
 * Initialize WASM memory for framebuffer sharing
 * (Legacy function - most of this logic is now in EmulatorCore)
 */
export function initFramebuffer(wasmModule, coreName, width, height) {
    const prefix = coreName === 'gb' ? 'gb_' : coreName === 'gbc' ? 'gbc_' : 'gba_';
    const getFramebufferPtr = wasmModule[`_${prefix}get_framebuffer`] ||
        wasmModule.exports?.[`${prefix}get_framebuffer`];

    if (!getFramebufferPtr) {
        throw new Error('WASM module does not export framebuffer function');
    }

    const fbPtr = getFramebufferPtr();
    const fbSize = width * height * 4;

    const memory = wasmModule.HEAPU8 || new Uint8Array(wasmModule.exports.memory.buffer);
    return new Uint8ClampedArray(memory.buffer, fbPtr, fbSize);
}

/**
 * Load a ROM file into WASM memory
 */
export function loadROMIntoWASM(wasmModule, coreName, romData) {
    const prefix = coreName === 'gb' ? 'gb_' : coreName === 'gbc' ? 'gbc_' : 'gba_';
    const loadRom = wasmModule[`_${prefix}load_rom`] ||
        wasmModule.exports?.[`${prefix}load_rom`];

    if (!loadRom) {
        throw new Error('WASM module does not export load_rom function');
    }

    const malloc = wasmModule._malloc || wasmModule.exports?.malloc;
    const free = wasmModule._free || wasmModule.exports?.free;

    if (!malloc || !free) {
        throw new Error('WASM module lacks memory management functions');
    }

    const romPtr = malloc(romData.length);
    const heap = wasmModule.HEAPU8 || new Uint8Array(wasmModule.exports.memory.buffer);
    heap.set(romData, romPtr);

    const result = loadRom(romPtr, romData.length);
    free(romPtr);

    return result;
}
