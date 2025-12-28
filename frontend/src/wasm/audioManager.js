/**
 * NeoBoy - AudioManager
 * 
 * Handles Web Audio API integration for the emulator.
 * Manages sample playback from the WASM core.
 */

export class AudioManager {
    constructor(sampleRate = 44100) {
        this.sampleRate = sampleRate;
        this.ctx = null;
        this.nextStartTime = 0;
        this.bufferSize = 4096;
        this.enabled = false;
    }

    /**
     * Initialize AudioContext (must be called after user interaction)
     */
    init() {
        if (this.ctx) return;

        try {
            this.ctx = new (window.AudioContext || window.webkitAudioContext)({
                sampleRate: this.sampleRate
            });
            this.nextStartTime = this.ctx.currentTime;
            this.enabled = true;
            console.log('AudioContext initialized at', this.sampleRate, 'Hz');
        } catch (e) {
            console.error('Failed to initialize AudioContext:', e);
        }
    }

    /**
     * Queue a buffer of samples for playback
     * @param {Float32Array} samples 
     */
    playSamples(samples) {
        if (!this.enabled || !this.ctx || !samples) return;

        // Resume if suspended (browser policy)
        if (this.ctx.state === 'suspended') {
            this.ctx.resume();
        }

        const buffer = this.ctx.createBuffer(1, samples.length, this.sampleRate);
        buffer.getChannelData(0).set(samples);

        const source = this.ctx.createBufferSource();
        source.buffer = buffer;
        source.connect(this.ctx.destination);

        // Schedule playback to avoid gaps/pops
        const currentTime = this.ctx.currentTime;
        if (this.nextStartTime < currentTime) {
            this.nextStartTime = currentTime + 0.01; // Small buffer
        }

        source.start(this.nextStartTime);
        this.nextStartTime += buffer.duration;
    }

    setEnabled(enabled) {
        this.enabled = enabled;
        if (!enabled && this.ctx && this.ctx.state === 'running') {
            this.ctx.suspend();
        } else if (enabled && this.ctx && this.ctx.state === 'suspended') {
            this.ctx.resume();
        }
    }

    cleanup() {
        if (this.ctx) {
            this.ctx.close();
            this.ctx = null;
        }
    }
}

export default AudioManager;
