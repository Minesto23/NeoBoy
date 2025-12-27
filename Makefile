# NeoBoy - Multi-Core Game Boy Emulator
# Makefile for building WASM cores

CC = emcc
CFLAGS = -O3 -s WASM=1 -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1
EXPORTED_FUNCTIONS = _malloc,_free

# Core directories
GB_DIR = wasm/core-gb
GBC_DIR = wasm/core-gbc
GBA_DIR = wasm/core-gba

# Output directory
OUT_DIR = frontend/public/wasm

# Source files
GB_SOURCES = $(GB_DIR)/cpu.c $(GB_DIR)/mmu.c $(GB_DIR)/ppu.c $(GB_DIR)/apu.c $(GB_DIR)/cartridge.c
GBC_SOURCES = $(GBC_DIR)/cpu.c $(GBC_DIR)/mmu.c $(GBC_DIR)/ppu.c $(GBC_DIR)/apu.c $(GBC_DIR)/cartridge.c
GBA_SOURCES = $(GBA_DIR)/cpu.c $(GBA_DIR)/mmu.c $(GBA_DIR)/ppu.c $(GBA_DIR)/apu.c $(GBA_DIR)/dma.c $(GBA_DIR)/cartridge.c

# Targets
all: gb gbc gba

gb:
	@echo "Building Game Boy core..."
	@mkdir -p $(OUT_DIR)
	$(CC) $(GB_SOURCES) $(CFLAGS) \
		-s EXPORT_NAME="NeoBoyGB" \
		-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
		-o $(OUT_DIR)/gb.js

gbc:
	@echo "Building Game Boy Color core..."
	@mkdir -p $(OUT_DIR)
	$(CC) $(GBC_SOURCES) $(CFLAGS) \
		-s EXPORT_NAME="NeoBoyGBC" \
		-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
		-o $(OUT_DIR)/gbc.js

gba:
	@echo "Building Game Boy Advance core..."
	@mkdir -p $(OUT_DIR)
	$(CC) $(GBA_SOURCES) $(CFLAGS) \
		-s EXPORT_NAME="NeoBoyGBA" \
		-s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
		-o $(OUT_DIR)/gba.js

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OUT_DIR)/*.js $(OUT_DIR)/*.wasm

frontend:
	@echo "Building React frontend..."
	cd frontend && npm install && npm run build

dev:
	@echo "Starting development server..."
	cd frontend && npm run dev

.PHONY: all gb gbc gba clean frontend dev
