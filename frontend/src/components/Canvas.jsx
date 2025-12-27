/**
 * NeoBoy - Canvas Component
 * 
 * Renders the emulator framebuffer at 60 FPS
 * Automatically scales to appropriate resolution based on core type
 */

import React, { useRef, useEffect } from 'react';
import './Canvas.css';

const CORE_RESOLUTIONS = {
    gb: { width: 160, height: 144 },
    gbc: { width: 160, height: 144 },
    gba: { width: 240, height: 160 }
};

function Canvas({ wasmInstance, coreType, isRunning }) {
    const canvasRef = useRef(null);
    const frameRef = useRef(null);

    const resolution = CORE_RESOLUTIONS[coreType];

    /**
     * Render framebuffer to canvas
     */
    const renderFrame = () => {
        if (!canvasRef.current || !wasmInstance.current) {
            return;
        }

        const canvas = canvasRef.current;
        const ctx = canvas.getContext('2d');

        // TODO: Get framebuffer from WASM
        // const framebuffer = wasmInstance.current.get_framebuffer();
        // const imageData = new ImageData(
        //   new Uint8ClampedArray(framebuffer),
        //   resolution.width,
        //   resolution.height
        // );

        // Placeholder: render a test pattern
        const imageData = ctx.createImageData(resolution.width, resolution.height);
        for (let i = 0; i < imageData.data.length; i += 4) {
            imageData.data[i + 0] = 224;  // R
            imageData.data[i + 1] = 248;  // G
            imageData.data[i + 2] = 208;  // B
            imageData.data[i + 3] = 255;  // A
        }

        ctx.putImageData(imageData, 0, 0);
    };

    useEffect(() => {
        if (isRunning) {
            const interval = setInterval(renderFrame, 16); // ~60 FPS
            return () => clearInterval(interval);
        }
    }, [isRunning, wasmInstance, coreType]);

    return (
        <div className="canvas-container">
            <canvas
                ref={canvasRef}
                width={resolution.width}
                height={resolution.height}
                className="emulator-canvas"
            />
        </div>
    );
}

export default Canvas;
