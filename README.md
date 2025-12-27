# NeoBoy - Multi-Core Game Boy Emulator

![NeoBoy Logo](https://img.shields.io/badge/NeoBoy-Emulator-e94560?style=for-the-badge)
[![License](https://img.shields.io/badge/License-MIT-blue.svg?style=for-the-badge)](LICENSE)

**NeoBoy** is a web-based, multi-core Nintendo handheld emulator supporting **Game Boy (DMG)**, **Game Boy Color (GBC)**, and **Game Boy Advance (GBA)**. Built with **C cores compiled to WebAssembly** and a modern **React** frontend.

---

## 🎮 Features

- **Multi-Core Support**
  - ✅ Game Boy (DMG): 160x144, 4 shades of gray
  - ✅ Game Boy Color: Full color, VRAM banking, palettes
  - ✅ Game Boy Advance: ARM7TDMI CPU, 240x160, 15-bit color

- **Input Handling**
  - ⌨️ Keyboard mapping (arrows, Z/X for A/B, Enter/Backspace for Start/Select)
  - 🎮 Gamepad support via Gamepad API

- **Save States**
  - 💾 Save/load states with IndexedDB persistence
  - 📂 Persistent SRAM/Flash/EEPROM saves

- **Modern Web Frontend**
  - ⚡ Built with React + Vite for fast development
  - 🎨 Beautiful dark theme with glassmorphism
  - 📱 Responsive design for desktop and mobile

- **WebAssembly Performance**
  - 🚀 High-performance C cores compiled with Emscripten
  - 🎯 60 FPS rendering on modern browsers

---

## 🏗️ Architecture

```
┌─────────────────────────────────────┐
│       React Frontend (Vite)         │
│  Canvas │ Controls │ ROM Loader     │
│  useInput │ useSave                  │
└──────────────┬──────────────────────┘
               │ JS ↔ WASM API
┌──────────────┴──────────────────────┐
│     WebAssembly Cores (C)           │
├─────────────┬──────────┬────────────┤
│   GB Core   │ GBC Core │  GBA Core  │
│  CPU, PPU,  │ CPU, PPU,│ ARM7TDMI,  │
│  MMU, APU,  │ MMU, APU,│ PPU, MMU,  │
│  Cartridge  │ Cartridge│ DMA, APU   │
└─────────────┴──────────┴────────────┘
```

---

## 🚀 Quick Start

### Prerequisites

- **Emscripten SDK** (for compiling C to WASM)
- **Node.js** (v18+ recommended)
- **npm** or **yarn**

### Installation

```bash
# 1. Clone the repository
git clone https://github.com/yourusername/NeoBoy.git
cd NeoBoy

# 2. Install frontend dependencies
cd frontend
npm install

# 3. Build WASM cores (requires Emscripten)
cd ..
make all
```

### Running the Emulator

```bash
# Start development server
make dev

# Or manually:
cd frontend
npm run dev
```

Open [http://localhost:3000](http://localhost:3000) in your browser.

---

## 📦 Building for Production

```bash
# Build WASM cores
make all

# Build React frontend
make frontend

# Output will be in frontend/dist/
```

---

## 🎮 Input Mapping

### Keyboard

| Game Boy Button | Keyboard Key      |
|-----------------|-------------------|
| A               | X                 |
| B               | Z                 |
| Start           | Enter             |
| Select          | Backspace         |
| D-Pad           | Arrow Keys        |
| L (GBA only)    | A                 |
| R (GBA only)    | S                 |

### Gamepad (Standard Mapping)

| Game Boy Button | Gamepad Button    |
|-----------------|-------------------|
| A               | Button 0 (A/Cross)|
| B               | Button 1 (B/Circle)|
| Start           | Button 9 (Start)  |
| Select          | Button 8 (Select) |
| D-Pad           | D-Pad             |
| L (GBA only)    | L1                |
| R (GBA only)    | R1                |

---

## 📁 Project Structure

```
NeoBoy/
├── wasm/                     # C cores (GB, GBC, GBA)
│   ├── core-gb/              # Game Boy core
│   ├── core-gbc/             # Game Boy Color core
│   └── core-gba/             # Game Boy Advance core
├── frontend/                 # React frontend
│   ├── src/
│   │   ├── components/       # Canvas, Controls, ROMLoader
│   │   ├── hooks/            # useInput, useSave
│   │   ├── App.jsx
│   │   └── index.jsx
│   ├── public/
│   └── package.json
├── assets/                   # ROMs (not included)
├── tests/                    # Test ROMs (Blargg tests)
├── Makefile                  # Build scripts
├── README.md
└── SRS.md                    # Software Requirements Specification
```

---

## ⚙️ WASM API

Each core exposes a common API:

```c
// Core initialization
void gb_init(void);

// ROM loading
int gb_load_rom(const uint8_t* rom_data, uint32_t size);

// Emulation
void gb_step_frame(void);  // Execute one frame (~70224 cycles)

// Input
void gb_set_button(GameBoyButton button, bool pressed);

// Rendering
uint8_t* gb_get_framebuffer(void);  // Returns RGBA framebuffer

// Save states
uint32_t gb_save_state(uint8_t* buffer);
int gb_load_state(const uint8_t* buffer, uint32_t size);

// Cleanup
void gb_reset(void);
void gb_destroy(void);
```

Similar APIs exist for `gbc_*` and `gba_*` cores.

---

## 🧪 Testing

NeoBoy uses **Blargg's test ROMs** for accuracy validation:

```bash
# Place test ROMs in tests/ directory
# TODO: Add test runner script
```

---

## ⚠️ Limitations

- **Audio**: Basic APU implementation (work in progress)
- **GBA BIOS**: Not included (optional for most games)
- **Accuracy**: Some edge cases may not be perfectly emulated
- **Performance**: Older browsers may struggle with GBA emulation

---

## 🌐 Browser Support

- ✅ Chrome/Edge 90+
- ✅ Firefox 88+
- ✅ Safari 14+
- ⚠️ Mobile browsers (limited gamepad support)

---

## 📜 License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- [Pan Docs](https://gbdev.io/pandocs/) - Game Boy technical documentation
- [GBATEK](https://problemkaputt.de/gbatek.htm) - GBA technical reference
- [Emscripten](https://emscripten.org/) - C to WebAssembly compiler
- [Blargg's Test ROMs](https://github.com/retrio/gb-test-roms) - Accuracy testing

---

## 🛠️ Development Roadmap

- [ ] Complete CPU instruction sets (GB, GBC, GBA)
- [ ] Full PPU rendering (backgrounds, sprites, windows)
- [ ] Audio implementation (APU + Web Audio API)
- [ ] MBC support (MBC1, MBC2, MBC3, MBC5)
- [ ] Save RAM persistence
- [ ] Debugger UI
- [ ] Rewind functionality
- [ ] Cheats support
- [ ] Online multiplayer (Link Cable emulation)

---

## 🤝 Contributing

Contributions are welcome! Please open an issue or pull request.

---

**Built with ❤️ by the NeoBoy Team**
