/**
 * NeoBoy - Canvas Component
 * 
 * Renders the emulator framebuffer at 60 FPS
 * Automatically scales to appropriate resolution based on core type
 */

import React, { useRef, useEffect, useMemo, useState } from 'react';
import { useAnimationFrame } from '../hooks/useAnimationFrame';
import { FramebufferManager } from '../wasm/framebufferManager';
import { AudioManager } from '../wasm/audioManager';
import './Canvas.css';

const CORE_RESOLUTIONS = {
    gb: { width: 160, height: 144 },
    gbc: { width: 160, height: 144 },
    gba: { width: 240, height: 160 }
};

function Canvas({ wasmCore, coreType, isRunning, onFPSUpdate }) {
    const canvasRef = useRef(null);
    const audioManagerRef = useRef(null);
    const [audioInitialized, setAudioInitialized] = useState(false);
    const resolution = CORE_RESOLUTIONS[coreType];

    // Initialize framebuffer manager
    const fbManager = useMemo(() => {
        return new FramebufferManager(resolution.width, resolution.height);
    }, [resolution.width, resolution.height]);

    // Initialize audio manager
    useEffect(() => {
        audioManagerRef.current = new AudioManager();
        return () => {
            if (audioManagerRef.current) {
                audioManagerRef.current.cleanup();
            }
        };
    }, []);

    const handleCanvasClick = () => {
        if (audioManagerRef.current && !audioInitialized) {
            audioManagerRef.current.init();
            setAudioInitialized(true);
        }
    };

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
        if (imageData) {
            ctx.putImageData(imageData, 0, 0);
        }

        // Handle Audio
        if (audioInitialized && audioManagerRef.current) {
            const samples = wasmCore.getAudioSamples();
            if (samples) {
                audioManagerRef.current.playSamples(samples);
            }
        }
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
        <div className="canvas-wrapper">
            <div className={`canvas-container ${isRunning ? 'running' : ''}`} onClick={handleCanvasClick}>
                {!audioInitialized && isRunning && (
                    <div className="audio-prompt">
                        <span>Click to enable audio</span>
                    </div>
                )}
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
                <div className="crt-overlay"></div>
                <div className="scanlines"></div>
            </div>
        </div>
    );
}

export default Canvas;
