CXX = g++
EMXX = em++
CXXFLAGS = -O2 -Wall -std=c++17
EMXXFLAGS = -O2 -sSTANDALONE_WASM -sWARN_ON_UNDEFINED_SYMBOLS=0

# Check dependencies
EMSCRIPTEN_CHECK := $(shell which $(EMXX) 2>/dev/null)
WASMEDGE_CHECK := $(shell which wasmedge 2>/dev/null)
ZLIB_PLUGIN_CHECK := $(shell ls $(HOME)/.wasmedge/plugin/libwasmedgePluginWasmEdgeZlib.* 2>/dev/null)

# Check Emscripten
ifeq ($(EMSCRIPTEN_CHECK),)
  $(warning Emscripten not found)
  WASM_AVAILABLE = 0
else
  WASM_AVAILABLE = 1
endif

# Check WasmEdge
ifeq ($(WASMEDGE_CHECK),)
  $(warning WasmEdge not found)
  WASM_AVAILABLE = 0
endif

# Check wasmedge-zlib plugin
ifeq ($(ZLIB_PLUGIN_CHECK),)
  $(warning wasmedge-zlib plugin not found)
  WASM_AVAILABLE = 0
endif

# Directories
BUILD_DIR = build
NATIVE_DIR = $(BUILD_DIR)/native
WASM_DIR = $(BUILD_DIR)/wasm

# Source files
SRCS = main.cpp
NATIVE_TARGET = $(NATIVE_DIR)/zlib_test
WASM_TARGET = $(WASM_DIR)/main.wasm

# Default target
ifeq ($(WASM_AVAILABLE), 1)
all: native wasm
else
all: native
endif

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(NATIVE_DIR): $(BUILD_DIR)
	mkdir -p $(NATIVE_DIR)

$(WASM_DIR): $(BUILD_DIR)
	mkdir -p $(WASM_DIR)

# Native build
native: $(NATIVE_DIR) $(NATIVE_TARGET)

$(NATIVE_TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lz

# WebAssembly build
wasm: $(WASM_DIR)
ifeq ($(WASM_AVAILABLE), 1)
	$(MAKE) $(WASM_TARGET)
else
	@echo "Skipping WebAssembly build - Emscripten not available"
endif

$(WASM_TARGET): $(SRCS)
	$(EMXX) $(EMXXFLAGS) $^ -o $@

# Run targets
run-native: native
	$(NATIVE_TARGET)

run-wasm: wasm
ifeq ($(WASM_AVAILABLE), 1)
	wasmedge $(WASM_TARGET)
else
	@echo "Cannot run WebAssembly - Emscripten not available"
endif

# Clean targets
clean:
	rm -rf $(BUILD_DIR)

clean-native:
	rm -rf $(NATIVE_DIR)

clean-wasm:
	rm -rf $(WASM_DIR)

# Help targets
help:
	@echo "Available targets:"
	@echo "  all        : Build both native and WebAssembly versions (default)"
	@echo "  native     : Build native version only"
	@echo "  wasm       : Build WebAssembly version only"
	@echo "  run-native : Build and run native version"
	@echo "  run-wasm   : Build and run WebAssembly version"
	@echo "  clean      : Remove all build artifacts"
	@echo "  clean-native: Remove native build artifacts"
	@echo "  clean-wasm : Remove WebAssembly build artifacts"
	@echo "  help       : Show this help message"

.PHONY: all native wasm run-native run-wasm clean clean-native clean-wasm help
