/**
 * NeoBoy - Main Application Component
 */

import React, { useState } from 'react';
import EmulatorHeader from './components/EmulatorHeader';
import Canvas from './components/Canvas';
import Controls from './components/Controls';
import ROMLoader from './components/ROMLoader';
import FPSDisplay from './components/FPSDisplay';
import SaveStateManager from './components/SaveStateManager';
import { useWASM } from './hooks/useWASM';
import { useInput } from './hooks/useInput';
import { useSave } from './hooks/useSave';
import './App.css';

function App() {
    const [coreType, setCoreType] = useState('gb');
    const [isRunning, setIsRunning] = useState(false);
    const [fps, setFps] = useState(0);
    const [isSaveOpen, setIsSaveOpen] = useState(false);

    // Initialize WASM core
    const {
        wasmCore,
        isLoading,
        error,
        loadROM,
        reset
    } = useWASM(coreType);

    // Initialize input handling
    useInput(wasmCore);

    // Initialize save/load persistence
    const { saveState, loadSavedState } = useSave(wasmCore, coreType);

    /**
     * Handle ROM file selection
     */
    const handleROMSelect = async (file) => {
        try {
            const success = await loadROM(file);
            if (success) {
                setIsRunning(true);
            }
        } catch (err) {
            alert(`Failed to load ROM: ${err.message}`);
        }
    };

    /**
     * Handle core type change
     */
    const handleCoreChange = (type) => {
        setIsRunning(false);
        setCoreType(type);
    };

    if (error) {
        return (
            <div className="app-error">
                <h2>Error Loading Emulator</h2>
                <p>{error}</p>
                <button onClick={() => window.location.reload()}>Retry</button>
            </div>
        );
    }

    return (
        <div className="app">
            <EmulatorHeader
                activeCore={coreType}
                onCoreChange={handleCoreChange}
            />

            <main className="app-main">
                <div className="emulator-container">
                    {isLoading && (
                        <div className="loading-overlay">
                            <div className="spinner"></div>
                            <p>Loading {coreType.toUpperCase()} Core...</p>
                        </div>
                    )}

                    <Canvas
                        wasmCore={wasmCore}
                        coreType={coreType}
                        isRunning={isRunning}
                        onFPSUpdate={setFps}
                    />

                    <FPSDisplay fps={fps} />
                </div>

                <Controls
                    isRunning={isRunning}
                    onTogglePlay={() => setIsRunning(!isRunning)}
                    onReset={reset}
                    onOpenSave={() => setIsSaveOpen(true)}
                />

                <ROMLoader onLoad={handleROMSelect} />
            </main>

            {isSaveOpen && (
                <SaveStateManager
                    onClose={() => setIsSaveOpen(false)}
                    onSave={saveState}
                    onLoad={loadSavedState}
                />
            )}

            <footer className="app-footer">
                <p>NeoBoy &copy; 2025 - All-in-one Web Emulator</p>
            </footer>
        </div>
    );
}

export default App;
