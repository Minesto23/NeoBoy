/**
 * NeoBoy - useWASM Hook
 * 
 * Custom React hook for WASM module lifecycle management
 * 
 * Features:
 * - Automatic WASM loading
 * - Module initialization
 * - Cleanup on unmount
 * - Error handling
 */

import { useState, useEffect, useCallback } from 'react';
import { loadWASMCore } from '../wasm/wasmLoader';
import EmulatorCore from '../wasm/wasmBindings';

/**
 * Hook for managing WASM emulator core
 * @param {string} coreName - Name of core to load ('gb', 'gbc', or 'gba')
 * @returns {object} WASM module state and controls
 */
export function useWASM(coreName = 'gb') {
    const [wasmCore, setWasmCore] = useState(null);
    const [isLoading, setIsLoading] = useState(true);
    const [error, setError] = useState(null);
    const [isInitialized, setIsInitialized] = useState(false);

    // Load WASM module on mount
    useEffect(() => {
        let mounted = true;

        async function initWASM() {
            try {
                setIsLoading(true);
                setError(null);

                console.log(`Loading WASM core: ${coreName}`);
                const wasmInstance = await loadWASMCore(coreName);

                if (!mounted) return;

                // Create core wrapper
                const core = new EmulatorCore(wasmInstance, coreName);
                core.initialize();

                setWasmCore(core);
                setIsInitialized(true);
                setIsLoading(false);

                console.log(`WASM core ${coreName} loaded successfully`);
            } catch (err) {
                if (!mounted) return;

                console.error('Failed to load WASM:', err);
                setError(err.message);
                setIsLoading(false);
            }
        }

        initWASM();

        // Cleanup on unmount
        return () => {
            mounted = false;
            if (wasmCore) {
                wasmCore.cleanup();
            }
        };
    }, [coreName]);

    /**
     * Load a ROM file
     */
    const loadROM = useCallback(async (romFile) => {
        if (!wasmCore) {
            throw new Error('WASM core not loaded');
        }

        try {
            // Read ROM file as array buffer
            const arrayBuffer = await romFile.arrayBuffer();
            const romData = new Uint8Array(arrayBuffer);

            // Load ROM into WASM
            const success = wasmCore.load(romData);

            if (!success) {
                throw new Error('Failed to load ROM');
            }

            console.log(`ROM loaded: ${romFile.name}`);
            return true;
        } catch (err) {
            console.error('Error loading ROM:', err);
            throw err;
        }
    }, [wasmCore]);

    /**
     * Reset the emulator
     */
    const reset = useCallback(() => {
        if (wasmCore) {
            wasmCore.resetCore();
        }
    }, [wasmCore]);

    /**
     * Save state
     */
    const saveState = useCallback(() => {
        if (!wasmCore) return null;
        return wasmCore.save();
    }, [wasmCore]);

    /**
     * Load state
     */
    const loadState = useCallback((saveData) => {
        if (!wasmCore) return false;
        return wasmCore.loadSave(saveData);
    }, [wasmCore]);

    return {
        wasmCore,
        isLoading,
        error,
        isInitialized,
        loadROM,
        reset,
        saveState,
        loadState
    };
}

export default useWASM;
