/**
 * NeoBoy - Canvas Component
 * 
 * Renders the emulator framebuffer at 60 FPS
 * Automatically scales to appropriate resolution based on core type
 */

import React, { useRef, useEffect, useMemo } from 'react';
import { useAnimationFrame } from '../hooks/useAnimationFrame';
import { FramebufferManager } from '../wasm/framebufferManager';
import './Canvas.css';

const CORE_RESOLUTIONS = {
    gb: { width: 160, height: 144 },
    gbc: { width: 160, height: 144 },
    gba: { width: 240, height: 160 }
};

function Canvas({ wasmCore, coreType, isRunning, onFPSUpdate }) {
    const canvasRef = useRef(null);
    const resolution = CORE_RESOLUTIONS[coreType];

    // Initialize framebuffer manager
    const fbManager = useMemo(() => {
        return new FramebufferManager(resolution.width, resolution.height);
    }, [resolution.width, resolution.height]);

    /**
     * Main rendering step
     */
    const renderStep = () => {
        if (!canvasRef.current || !wasmCore || !isRunning) {
            return;
        }

        // Step the emulator frame
        wasmCore.step();

        // Get framebuffer and draw to canvas
        const canvas = canvasRef.current;
        const ctx = canvas.getContext('2d');

        const imageData = wasmCore.getFramebufferImageData(resolution.width, resolution.height);
        ctx.putImageData(imageData, 0, 0);
    };

    // Use optimized animation frame hook
    const { fps } = useAnimationFrame(renderStep, isRunning);

    // Report FPS back to parent if needed
    useEffect(() => {
        if (onFPSUpdate) {
            onFPSUpdate(fps);
        }
    }, [fps, onFPSUpdate]);

    return (
        <div className="canvas-container">
            <canvas
                ref={canvasRef}
                width={resolution.width}
                height={resolution.height}
                className="emulator-canvas"
                style={{
                    imageRendering: 'pixelated',
                    width: '100%',
                    height: 'auto',
                    maxWidth: `${resolution.width * 6}px`
                }}
            />
        </div>
    );
}

export default Canvas;
