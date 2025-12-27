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

function Controls({ isRunning, onPlayPause, onSave, onLoad, fps }) {
    return (
        <div className="controls">
            <div className="control-group">
                <button
                    className="control-button primary"
                    onClick={onPlayPause}
                >
                    {isRunning ? '⏸️ Pause' : '▶️ Play'}
                </button>
            </div>

            <div className="control-group">
                <button
                    className="control-button"
                    onClick={onSave}
                    disabled={!isRunning}
                >
                    💾 Save State
                </button>
                <button
                    className="control-button"
                    onClick={onLoad}
                >
                    📂 Load State
                </button>
            </div>

            <div className="fps-display">
                FPS: {fps.toFixed(1)}
            </div>
        </div>
    );
}

export default Controls;
