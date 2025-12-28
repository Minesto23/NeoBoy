/**
 * NeoBoy - Input Hook
 * 
 * Handles keyboard and gamepad input
 * Maps inputs to Game Boy buttons and sends to WASM
 */

import { useEffect } from 'react';

// Keyboard mapping
const KEY_MAPPING = {
    'KeyX': 0,        // A
    'KeyZ': 1,        // B
    'Backspace': 2,   // Select
    'Enter': 3,       // Start
    'ArrowRight': 4,  // Right
    'ArrowLeft': 5,   // Left
    'ArrowUp': 6,     // Up
    'ArrowDown': 7,   // Down
    'KeyA': 8,        // L (GBA)
    'KeyS': 9         // R (GBA)
};

export function useInput(wasmCore) {
    useEffect(() => {
        const handleKeyDown = (event) => {
            const button = KEY_MAPPING[event.code];
            if (button !== undefined && wasmCore) {
                wasmCore.setButtonState(button, true);
                event.preventDefault();
            }
        };

        const handleKeyUp = (event) => {
            const button = KEY_MAPPING[event.code];
            if (button !== undefined && wasmCore) {
                wasmCore.setButtonState(button, false);
                event.preventDefault();
            }
        };

        /**
         * Gamepad support using Gamepad API
         */
        const pollGamepad = () => {
            const gamepads = navigator.getGamepads();

            for (let i = 0; i < gamepads.length; i++) {
                const gamepad = gamepads[i];
                if (!gamepad) continue;

                const buttonMap = {
                    0: 0,   // A
                    1: 1,   // B
                    8: 2,   // Select
                    9: 3,   // Start
                    15: 4,  // Right
                    14: 5,  // Left
                    12: 6,  // Up
                    13: 7,  // Down
                    4: 8,   // L
                    5: 9    // R
                };

                for (const [gpButton, emuButton] of Object.entries(buttonMap)) {
                    if (gamepad.buttons[gpButton]?.pressed && wasmCore) {
                        wasmCore.setButtonState(emuButton, true);
                    } else if (wasmCore) {
                        wasmCore.setButtonState(emuButton, false);
                    }
                }
            }
        };

        // Set up keyboard listeners
        window.addEventListener('keydown', handleKeyDown);
        window.addEventListener('keyup', handleKeyUp);

        // Poll gamepad at 60 FPS
        const gamepadInterval = setInterval(pollGamepad, 16);

        return () => {
            window.removeEventListener('keydown', handleKeyDown);
            window.removeEventListener('keyup', handleKeyUp);
            clearInterval(gamepadInterval);
        };
    }, [wasmCore]);
}
