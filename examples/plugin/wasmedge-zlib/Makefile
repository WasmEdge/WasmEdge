.PHONY: help
help:
	@echo "📘 Makefile Help"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Run tool checks and build both native and WASM targets"
	@echo "  check-all-tools  - Check all required tools for bindings, native, wasm, and runner"
	@echo "  check-bindings   - Check required tools for binding generation"
	@echo "  check-native     - Check tools required for native build"
	@echo "  check-wasm       - Check tools required for wasm build"
	@echo "  check-run-wasm   - Check Wasm runner (wasmedge)"
	@echo "  native           - Build the native executable"
	@echo "  run-native       - Run the native executable"
	@echo "  wasm             - Build the WebAssembly module"
	@echo "  run-wasm         - Run the WebAssembly module with WasmEdge"
	@echo "  clean            - Remove all build and generated files"
	@echo "  help             - Show this help message"

# -------- Configuration --------
WIT_FOLDER := wit
BINDINGS_DIR := bindings
BINDINGS_C := $(BINDINGS_DIR)/zlib_component.c
BINDINGS_H := $(BINDINGS_DIR)/zlib_component.h
BINDINGS_COMPONENT_TYPE_O := $(BINDINGS_DIR)/zlib_component_component_type.o
MAIN_CPP := main.cpp

BUILD_NATIVE_DIR := build/native
BUILD_WASM_DIR := build/wasm
NATIVE_EXE := $(BUILD_NATIVE_DIR)/zlib_test_native
WASM_EXE := $(BUILD_WASM_DIR)/main.wasm

CFLAGS := -O2 -Wall -Wextra
CPPFLAGS := -std=c++17
LDFLAGS_NATIVE := -lz

# -------- Compiler Detection --------
ifeq (, $(shell which clang))
	CC := gcc
	CXX := g++
else
	CC := clang
	CXX := clang++
endif

# -------- Dependency Lists --------
DEPS_BINDING := wit-bindgen awk
DEPS_NATIVE  := $(CC) $(CXX) awk
DEPS_WASM    := emcc em++ awk
DEPS_RUN_WASM := wasmedge

# -------- WIT Source Files --------
WIT_FILES := $(shell find $(WIT_FOLDER) -name '*.wit' -print 2>/dev/null)
ifeq ($(strip $(WIT_FILES)),)
  WIT_SOURCES := $(WIT_FOLDER)
else
  WIT_SOURCES := $(WIT_FILES)
endif

# -------- Default Target --------
all: check-all-tools native wasm

# -------- Dependency Checks --------
.PHONY: check-bindings check-native check-wasm check-run-wasm check-all-tools
check-bindings:
	@echo "🔍 Checking binding-generation tools..."
	@for cmd in $(DEPS_BINDING); do \
	  command -v $$cmd >/dev/null 2>&1 || { echo "Error: '$$cmd' not found in PATH."; exit 1; }; \
	done
	@echo "✔️  Binding tools OK."

check-native:
	@echo "🔍 Checking native-build tools..."
	@for cmd in $(DEPS_NATIVE); do \
	  command -v $$cmd >/dev/null 2>&1 || { echo "Error: '$$cmd' not found in PATH."; exit 1; }; \
	done
	@echo "✔️  Native build tools OK."

check-wasm:
	@echo "🔍 Checking wasm-build tools..."
	@for cmd in $(DEPS_WASM); do \
	  command -v $$cmd >/dev/null 2>&1 || { echo "Error: '$$cmd' not found in PATH."; exit 1; }; \
	done
	@echo "✔️  Wasm build tools OK."

check-run-wasm:
	@echo "🔍 Checking Wasm runner..."
	@for cmd in $(DEPS_RUN_WASM); do \
	  command -v $$cmd >/dev/null 2>&1 || { echo "Error: '$$cmd' not found in PATH."; exit 1; }; \
	done
	@echo "✔️  Wasm runner OK."

check-all-tools: check-bindings check-native check-wasm check-run-wasm

# -------- Binding Generation & Sanitization --------
$(BINDINGS_C) $(BINDINGS_H) $(BINDINGS_COMPONENT_TYPE_O): $(WIT_SOURCES)
	@$(MAKE) -s check-bindings
	@echo "Generating WIT bindings (produces .c, .h, component_type.o)..."
	mkdir -p $(BINDINGS_DIR)
	wit-bindgen c $(WIT_FOLDER) --out-dir $(BINDINGS_DIR)
	@echo "Sanitizing export names in $(BINDINGS_C)..."
	@awk -v sanitize_chars_regex='[:/@#%.]' '\
	{\
		line = $$0; processed_line = 0;\
		prefix_end_marker = "__export_name__(\"";\
		start_pos = index(line, prefix_end_marker);\
		if (start_pos > 0) {\
			name_start_pos = start_pos + length(prefix_end_marker);\
			name_and_suffix_part = substr(line, name_start_pos);\
			closing_quote_pos_in_suffix = index(name_and_suffix_part, "\"");\
			if (closing_quote_pos_in_suffix > 0) {\
				name_to_sanitize = substr(name_and_suffix_part, 1, closing_quote_pos_in_suffix - 1);\
				prefix = substr(line, 1, name_start_pos - 1);\
				suffix = substr(name_and_suffix_part, closing_quote_pos_in_suffix);\
				sanitized_name = name_to_sanitize;\
				gsub(sanitize_chars_regex, "_", sanitized_name);\
				print prefix sanitized_name suffix;\
				processed_line = 1;\
			}\
		}\
		if (processed_line == 0) print line;\
	}' $(BINDINGS_C) > $(BINDINGS_C).tmp && mv $(BINDINGS_C).tmp $(BINDINGS_C)

# -------- Native Build --------
native: check-native $(NATIVE_EXE)

$(BUILD_NATIVE_DIR)/main_native.o: $(MAIN_CPP)
	mkdir -p $(BUILD_NATIVE_DIR)
	$(CXX) $(CFLAGS) $(CPPFLAGS) -c $(MAIN_CPP) -o $@

$(NATIVE_EXE): $(BUILD_NATIVE_DIR)/main_native.o
	@echo "Building native executable..."
	$(CXX) $^ -o $@ $(LDFLAGS_NATIVE)
	@echo "✔️  Native build complete: $@"

.PHONY: run-native
run-native: $(NATIVE_EXE)
	@echo "▶️  Running native executable..."
	$(NATIVE_EXE)

# -------- WASM Build --------
wasm: check-wasm $(WASM_EXE)

$(BUILD_WASM_DIR)/main.o: $(MAIN_CPP) $(BINDINGS_H)
	mkdir -p $(BUILD_WASM_DIR)
	em++ $(CFLAGS) -sWARN_ON_UNDEFINED_SYMBOLS=0 $(CPPFLAGS) -I$(BINDINGS_DIR) -c $(MAIN_CPP) -o $@

$(BUILD_WASM_DIR)/zlib_component_bindings.o: $(BINDINGS_C) $(BINDINGS_H)
	mkdir -p $(BUILD_WASM_DIR)
	emcc $(CFLAGS) -sWARN_ON_UNDEFINED_SYMBOLS=0 -I$(BINDINGS_DIR) -c $(BINDINGS_C) -o $@

$(WASM_EXE): $(BUILD_WASM_DIR)/main.o $(BUILD_WASM_DIR)/zlib_component_bindings.o $(BINDINGS_COMPONENT_TYPE_O)
	@echo "Building WebAssembly module..."
	em++ $(CFLAGS) -sSTANDALONE_WASM -sWARN_ON_UNDEFINED_SYMBOLS=0 \
		$(BUILD_WASM_DIR)/main.o \
		$(BUILD_WASM_DIR)/zlib_component_bindings.o \
		$(BINDINGS_COMPONENT_TYPE_O) \
		-o $@
	@echo "✔️  WASM build complete: $@"

.PHONY: run-wasm
run-wasm: check-run-wasm wasm
	@echo "▶️  Running WebAssembly module via WasmEdge..."
	wasmedge $(WASM_EXE)

# -------- Clean --------
.PHONY: clean
clean:
	rm -rf $(BUILD_NATIVE_DIR) $(BUILD_WASM_DIR) $(BINDINGS_DIR)
