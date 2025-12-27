/**
 * NeoBoy - Main React Application
 * 
 * Main component orchestrating the emulator UI
 * - Core selection (GB, GBC, GBA)
 * - Canvas rendering
 * - Input handling
 * - Save/load functionality
 */

import React, { useState, useEffect, useRef } from 'react';
import Canvas from './components/Canvas';
import Controls from './components/Controls';
import ROMLoader from './components/ROMLoader';
import { useInput } from './hooks/useInput';
import { useSave } from './hooks/useSave';
import './App.css';

function App() {
    const [coreType, setCoreType] = useState('gb'); // 'gb', 'gbc', 'gba'
    const [isRunning, setIsRunning] = useState(false);
    const [fps, setFps] = useState(0);
    const wasmInstance = useRef(null);
    const animationFrame = useRef(null);

    // Initialize input handling
    useInput(wasmInstance);

    // Initialize save/load
    const { saveState, loadState } = useSave(wasmInstance, coreType);

    /**
     * Load WASM module for selected core
     */
    const loadCore = async (type) => {
        try {
            // TODO: Load appropriate WASM module based on core type
            // const module = await import(`./wasm/${type}.js`);
            // wasmInstance.current = await module.default();
            console.log(`Loading ${type.toUpperCase()} core...`);
        } catch (error) {
            console.error('Failed to load WASM core:', error);
        }
    };

    useEffect(() => {
        loadCore(coreType);
    }, [coreType]);

    /**
     * Load ROM file
     */
    const handleROMLoad = async (file) => {
        const buffer = await file.arrayBuffer();
        const data = new Uint8Array(buffer);

        // TODO: Call WASM load_rom function
        // wasmInstance.current.load_rom(data);

        console.log(`Loaded ROM: ${file.name} (${data.length} bytes)`);
        setIsRunning(true);
    };

    /**
     * Main emulation loop (60 FPS)
     */
    const emulationLoop = (timestamp) => {
        if (!isRunning || !wasmInstance.current) {
            return;
        }

        // TODO: Call WASM step_frame()
        // wasmInstance.current.step_frame();

        // Calculate FPS
        // TODO: Implement FPS counter

        animationFrame.current = requestAnimationFrame(emulationLoop);
    };

    useEffect(() => {
        if (isRunning) {
            animationFrame.current = requestAnimationFrame(emulationLoop);
        } else {
            if (animationFrame.current) {
                cancelAnimationFrame(animationFrame.current);
            }
        }

        return () => {
            if (animationFrame.current) {
                cancelAnimationFrame(animationFrame.current);
            }
        };
    }, [isRunning]);

    return (
        <div className="app">
            <header className="app-header">
                <h1>NeoBoy Emulator</h1>
                <div className="core-selector">
                    <button
                        className={coreType === 'gb' ? 'active' : ''}
                        onClick={() => setCoreType('gb')}
                    >
                        Game Boy
                    </button>
                    <button
                        className={coreType === 'gbc' ? 'active' : ''}
                        onClick={() => setCoreType('gbc')}
                    >
                        Game Boy Color
                    </button>
                    <button
                        className={coreType === 'gba' ? 'active' : ''}
                        onClick={() => setCoreType('gba')}
                    >
                        Game Boy Advance
                    </button>
                </div>
            </header>

            <main className="app-main">
                <Canvas
                    wasmInstance={wasmInstance}
                    coreType={coreType}
                    isRunning={isRunning}
                />

                <Controls
                    isRunning={isRunning}
                    onPlayPause={() => setIsRunning(!isRunning)}
                    onSave={() => saveState()}
                    onLoad={() => loadState()}
                    fps={fps}
                />
            </main>

            <ROMLoader onLoad={handleROMLoad} />
        </div>
    );
}

export default App;
