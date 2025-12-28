/**
 * NeoBoy - FPS Display Component
 * 
 * Shows real-time FPS counter
 */

import React from 'react';
import './FPSDisplay.css';

export function FPSDisplay({ fps }) {
    const getFpsColor = (fps) => {
        if (fps >= 55) return '#4ade80'; // Green - good
        if (fps >= 45) return '#fbbf24'; // Yellow - okay
        return '#f87171';                // Red - poor
    };

    return (
        <div className="fps-display">
            <span className="fps-label">FPS:</span>
            <span
                className="fps-value"
                style={{ color: getFpsColor(fps) }}
            >
                {fps}
            </span>
        </div>
    );
}

export default FPSDisplay;
