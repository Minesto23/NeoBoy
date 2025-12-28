/**
 * NeoBoy - useAnimationFrame Hook
 * 
 * Custom hook for managing 60 FPS rendering loop
 * 
 * Features:
 * - RequestAnimationFrame-based loop
 * - FPS tracking
 * - Automatic cleanup
 * - Pause/resume support
 */

import { useEffect, useRef, useCallback, useState } from 'react';

/**
 * Hook for managing 60 FPS animation loop
 * @param {Function} callback - Function to call each frame
 * @param {boolean} isActive - Whether the loop should be running
 * @returns {object} FPS counter and controls
 */
export function useAnimationFrame(callback, isActive = true) {
    const requestRef = useRef();
    const previousTimeRef = useRef();
    const [fps, setFps] = useState(0);
    const fpsCounterRef = useRef({ frames: 0, lastTime: performance.now() });

    const animate = useCallback((time) => {
        if (previousTimeRef.current !== undefined) {
            const deltaTime = time - previousTimeRef.current;

            // Limit to ~60 FPS (16.66ms per frame)
            // Even if the monitor is 144Hz/165Hz, we only step the emulator every 16ms
            if (deltaTime >= 16.6) {
                // Call the callback with delta time
                callback(deltaTime);

                // Update FPS counter
                fpsCounterRef.current.frames++;
                const elapsed = time - fpsCounterRef.current.lastTime;

                if (elapsed >= 1000) { // Update FPS every second
                    const currentFps = Math.round(
                        (fpsCounterRef.current.frames * 1000) / elapsed
                    );
                    setFps(currentFps);
                    fpsCounterRef.current.frames = 0;
                    fpsCounterRef.current.lastTime = time;
                }

                // Update previous time only when we actually step
                previousTimeRef.current = time;
            }
        } else {
            previousTimeRef.current = time;
        }

        requestRef.current = requestAnimationFrame(animate);
    }, [callback]);

    useEffect(() => {
        if (isActive) {
            requestRef.current = requestAnimationFrame(animate);
            return () => {
                if (requestRef.current) {
                    cancelAnimationFrame(requestRef.current);
                }
            };
        } else {
            if (requestRef.current) {
                cancelAnimationFrame(requestRef.current);
            }
            previousTimeRef.current = undefined;
        }
    }, [isActive, animate]);

    const pause = useCallback(() => {
        if (requestRef.current) {
            cancelAnimationFrame(requestRef.current);
            requestRef.current = null;
        }
    }, []);

    const resume = useCallback(() => {
        if (!requestRef.current && isActive) {
            previousTimeRef.current = undefined;
            requestRef.current = requestAnimationFrame(animate);
        }
    }, [isActive, animate]);

    return {
        fps,
        pause,
        resume
    };
}

export default useAnimationFrame;
