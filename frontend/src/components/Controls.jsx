/**
 * NeoBoy - Controls Component
 * 
 * UI controls for emulator
 * - Play/Pause
 * - Save/Load state
 * - FPS display
 */

import React from 'react';
import './Controls.css';

function Controls({ isRunning, onTogglePlay, onReset, onOpenSave }) {
    return (
        <div className="controls">
            <div className="control-group">
                <button
                    className={`control-button primary ${isRunning ? 'running' : 'paused'}`}
                    onClick={onTogglePlay}
                >
                    {isRunning ? '⏸️ Pause' : '▶️ Play'}
                </button>
                <button
                    className="control-button"
                    onClick={onReset}
                >
                    🔄 Reset
                </button>
            </div>

            <div className="control-group">
                <button
                    className="control-button secondary"
                    onClick={onOpenSave}
                >
                    💾 Save / Load
                </button>
            </div>
        </div>
    );
}

export default Controls;
