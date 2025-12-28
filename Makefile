# NeoBoy - Multi-Core Game Boy Emulator
# Makefile for building WASM cores

CC = emcc

# Emscripten flags
# -s EXPORT_ES6=1 transforms the output into an ES6 module
# -s MODULARIZE=1 wraps the JS Glue in a function/module
EMCC_FLAGS = -O3 -s WASM=1 -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1 \
             -s EXPORT_ES6=1 \
             -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "getValue", "setValue", "HEAP8", "HEAPU8", "HEAP16", "HEAPU16", "HEAP32", "HEAPU32", "HEAPF32", "HEAPF64"]' \
             -s ENVIRONMENT='web'

# Core directories
GB_DIR = wasm/core-gb
GBC_DIR = wasm/core-gbc
GBA_DIR = wasm/core-gba

# Output directory (moved to src for Vite integration)
OUT_DIR = frontend/src/wasm/generated

# Source files
GB_SOURCES = $(GB_DIR)/cpu.c $(GB_DIR)/mmu.c $(GB_DIR)/ppu.c $(GB_DIR)/apu.c $(GB_DIR)/cartridge.c $(GB_DIR)/gb.c
GBC_SOURCES = $(GBC_DIR)/cpu.c $(GBC_DIR)/mmu.c $(GBC_DIR)/ppu.c $(GBC_DIR)/apu.c $(GBC_DIR)/cartridge.c $(GBC_DIR)/gbc.c
GBA_SOURCES = $(GBA_DIR)/cpu.c $(GBA_DIR)/mmu.c $(GBA_DIR)/ppu.c $(GBA_DIR)/apu.c $(GBA_DIR)/dma.c $(GBA_DIR)/cartridge.c $(GBA_DIR)/gba.c

# Exported functions (keep _ prefix for EMCC)
GB_EXPORTS = ["_malloc","_free","_gb_init","_gb_load_rom","_gb_step_frame","_gb_set_button","_gb_get_framebuffer","_gb_get_audio_buffer","_gb_get_audio_buffer_size","_gb_save_state","_gb_load_state","_gb_reset","_gb_destroy"]
GBC_EXPORTS = ["_malloc","_free","_gbc_init","_gbc_load_rom","_gbc_step_frame","_gbc_set_button","_gbc_get_framebuffer","_gbc_save_state","_gbc_load_state","_gbc_reset","_gbc_destroy"]
GBA_EXPORTS = ["_malloc","_free","_gba_init","_gba_load_rom","_gba_step_frame","_gba_set_button","_gba_get_framebuffer","_gba_save_state","_gba_load_state","_gba_reset","_gba_destroy"]

# Targets
all: gb gbc gba

gb:
	@echo "Building Game Boy core..."
	@mkdir -p $(OUT_DIR)
	$(CC) $(GB_SOURCES) $(EMCC_FLAGS) \
		-s EXPORT_NAME="NeoBoyGB" \
		-s EXPORTED_FUNCTIONS='$(GB_EXPORTS)' \
		-o $(OUT_DIR)/gb.js

gbc:
	@echo "Building Game Boy Color core..."
	@mkdir -p $(OUT_DIR)
	$(CC) $(GBC_SOURCES) $(EMCC_FLAGS) \
		-s EXPORT_NAME="NeoBoyGBC" \
		-s EXPORTED_FUNCTIONS='$(GBC_EXPORTS)' \
		-o $(OUT_DIR)/gbc.js

gba:
	@echo "Building Game Boy Advance core..."
	@mkdir -p $(OUT_DIR)
	$(CC) $(GBA_SOURCES) $(EMCC_FLAGS) \
		-s EXPORT_NAME="NeoBoyGBA" \
		-s EXPORTED_FUNCTIONS='$(GBA_EXPORTS)' \
		-o $(OUT_DIR)/gba.js

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OUT_DIR)/*.js $(OUT_DIR)/*.wasm

.PHONY: all gb gbc gba clean
