/**
 * NeoBoy - ROM Loader Component
 * 
 * File picker for loading ROM files
 * Supports drag-and-drop and file selection
 */

import React, { useState } from 'react';
import './ROMLoader.css';

function ROMLoader({ onLoad }) {
    const [dragging, setDragging] = useState(false);

    const handleFileSelect = (event) => {
        const file = event.target.files?.[0];
        if (file) {
            onLoad(file);
        }
    };

    const handleDrop = (event) => {
        event.preventDefault();
        setDragging(false);

        const file = event.dataTransfer.files?.[0];
        if (file) {
            onLoad(file);
        }
    };

    const handleDragOver = (event) => {
        event.preventDefault();
        setDragging(true);
    };

    const handleDragLeave = () => {
        setDragging(false);
    };

    return (
        <div
            className={`rom-loader ${dragging ? 'dragging' : ''}`}
            onDrop={handleDrop}
            onDragOver={handleDragOver}
            onDragLeave={handleDragLeave}
        >
            <div className="rom-loader-content">
                <h3>Load ROM</h3>
                <p>Drag and drop a ROM file here, or click to select</p>
                <input
                    type="file"
                    accept=".gb,.gbc,.gba"
                    onChange={handleFileSelect}
                    id="rom-file-input"
                    className="rom-file-input"
                />
                <label htmlFor="rom-file-input" className="rom-file-label">
                    Choose File
                </label>
            </div>
        </div>
    );
}

export default ROMLoader;
