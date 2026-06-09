## Project Overview

WasmEdge is a C++17 WebAssembly runtime. It loads, validates, and executes `.wasm` modules.

Key directories:

| Path | Purpose |
|------|---------|
| `include/` | Public headers (mirrors `lib/` structure) |
| `lib/` | Core runtime: loader, validator, executor, VM |
| `plugins/` | Optional host-function plugins |
| `test/` | Google Test suites (mirrors `lib/` structure) |
| `tools/` | CLI binaries (`wasmedge`, `wasmedgec`) |

Entry points: `include/vm/vm.h` (runtime), `include/plugin/plugin.h` (plugin API).

### Core Runtime (`include/` + `lib/`)

The `include/` and `lib/` directories mirror each other:

| Module | Purpose |
|--------|---------|
| `loader/` | Parses `.wasm` binary into AST |
| `validator/` | Validates Wasm module against spec |
| `executor/` | Interprets/executes Wasm instructions |
| `vm/` | Orchestrates the full pipeline (load → validate → execute) |
| `aot/` + `llvm/` | AOT/JIT compilation via LLVM |
| `ast/` | AST node definitions for Wasm modules |
| `common/` | Shared types: `ErrCode`, `Expected<T>`, `Span`, config enums |
| `host/` | WASI and host function implementations |
| `api/` | C API surface (`wasmedge.h`) |
| `runtime/` | Runtime instances (memory, table, global, module) |
| `plugin/` | Plugin loading infrastructure |
| `system/` | OS abstraction (mmap, signals) |
| `driver/` | CLI driver for `wasmedge` / `wasmedgec` |
| `po/` | Program options parsing |

## Prerequisites

- CMake 3.18+
- C++17 compiler (GCC 11+, Clang 13+, or MSVC 19.29+)
- Optional: LLVM 17+ (for AOT/JIT, controlled by `WASMEDGE_USE_LLVM`)

## Code Style

- C++17 standard
- Follow LLVM coding style (enforced via `.clang-format` and `.clang-tidy`)
- 2-space indentation, no tabs; UTF-8 encoding; LF line endings
- CamelCase for classes, functions, variables, and parameters
- All files must end with a newline and have no trailing whitespace, run `lineguard` to check
- Do not add inline comments explaining the change
- Use `sv` suffix for string literals in spdlog calls (e.g., `spdlog::info("message"sv)`)

### Header File Format

```cpp
#pragma once

// ... includes ...

namespace WasmEdge {
namespace SubModule {
// ...
} // namespace SubModule
} // namespace WasmEdge
```

### Error Handling

- Use `Expect<T>` (alias for `Expected<T, ErrCode>`) for fallible return types
- Return errors with `Unexpect(ErrCode)` or `Unexpect(ErrCode::Value::...)`
- Propagate errors with `EXPECTED_TRY(expr)` macro (returns early on failure)

## Building

```bash
cmake -S . -B build -DWASMEDGE_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

On macOS, replace `$(nproc)` with `$(sysctl -n hw.logicalcpu)`.

### Core CMake Options

| Option | Default | Purpose |
|--------|---------|---------|
| `WASMEDGE_BUILD_TESTS` | OFF | Enable unit tests |
| `WASMEDGE_BUILD_TOOLS` | ON | Build CLI tools |
| `WASMEDGE_BUILD_PLUGINS` | ON | Build enabled plugins |
| `WASMEDGE_USE_LLVM` | ON | LLVM-based AOT/JIT compilation |
| `WASMEDGE_BUILD_SHARED_LIB` | ON | Build shared library |
| `WASMEDGE_BUILD_STATIC_LIB` | OFF | Build static library |

### Plugin CMake Flags

Each plugin has a `WASMEDGE_PLUGIN_<NAME>` flag (all OFF by default except `WASI_LOGGING`):

`WASI_CRYPTO`, `WASI_HTTP`, `WASI_LOGGING`, `WASI_NN_BACKEND`, `WASI_POLL`, `WASM_BPF`, `FFMPEG`, `IMAGE`, `LLMC`, `OCR`, `OPENCVMINI`, `PROCESS`, `STABLEDIFFUSION`, `TENSORFLOW`, `TENSORFLOWLITE`, `ZLIB`

Build with a specific plugin enabled:

```bash
cmake -S . -B build -DWASMEDGE_PLUGIN_IMAGE=ON -DWASMEDGE_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

## Build Artifacts

After building, key paths are:
- Binary: `build/tools/wasmedge/wasmedge`
- Libraries: `build/lib/api/`
- Plugins: `build/plugins/<plugin-name>/`

To run with plugins:
```bash
WASMEDGE_PLUGIN_PATH=build/plugins ./build/tools/wasmedge/wasmedge <wasm-file>
```

## Testing

Tests use Google Test framework:
```bash
cmake --build build --target test
```

Run a specific test:
```bash
cd build && ctest -R <test-name-regex>
```

Test files are located in `test/` with structure mirroring `lib/`. Plugin tests live in `test/plugins/`.

## Plugin System

Plugins add host functions that Wasm modules can call. See `plugins/wasmedge_image/` for a simple example.

### File Structure

A typical Module-based plugin has these files:

- `base.h` — CRTP host function base class
- `env.h` / `env.cpp` — environment state + plugin descriptor
- `func.h` / `func.cpp` — host function implementations
- `module.h` / `module.cpp` — module instance, registers host functions
- `CMakeLists.txt` — build configuration

### Key Patterns

- **CRTP host function base**: Each plugin defines a `Func<T>` class inheriting `Runtime::HostFunction<T>` that holds a reference to the plugin's environment. See `plugins/wasmedge_image/image_base.h`.
- **Plugin descriptor**: A `PluginDescriptor` struct in `env.cpp` declares the plugin name, version, and module factory. Must call `EXPORT_GET_DESCRIPTOR(Descriptor)`. See `plugins/wasmedge_image/image_env.cpp`.
- **Module registration**: The module constructor calls `addHostFunc(...)` for each exported function. See `plugins/wasmedge_image/image_module.cpp`.

### Component-Based Plugins

Some plugins (e.g., `wasi_poll`) use the Component Model API instead:

- Base class: `Runtime::Component::HostFunction<T>` (instead of `Runtime::HostFunction<T>`)
- Module: `Runtime::Instance::ComponentInstance` (instead of `ModuleInstance`)
- Descriptor uses `ComponentCount` / `ComponentDescriptions` (instead of `ModuleCount` / `ModuleDescriptions`)

### Plugin CMake Pattern

```cmake
wasmedge_add_library(wasmedgePluginMyPlugin SHARED
  env.cpp func.cpp module.cpp
)
target_link_libraries(wasmedgePluginMyPlugin PRIVATE wasmedge_shared)
```

Use `wasmedgeCAPI` instead of `wasmedge_shared` when `WASMEDGE_LINK_PLUGINS_STATIC` is ON.

## Adding New Files

- Source files: Add to the appropriate `CMakeLists.txt` in the directory
- Test files: Add to `test/` with corresponding CMake registration
- Header files: Place in `include/` with matching namespace structure
- New plugins require: an option in root `CMakeLists.txt` and a conditional entry in `plugins/CMakeLists.txt`

After adding files, verify the build works.

## Git Commits

Use Conventional Commits format with DCO sign-off:

```
<type>: <description>

Signed-off-by: Your Name <email@example.com>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`, `ci`

Rules:
- Header max 100 characters
- Each logical change gets its own commit
- Sign-off required: `git commit -s`
- Use `commitlint` for validation

### AI Assistance Disclosure

Use the `Assisted-by:` trailer for the AI assistance disclosure.

The `Assisted-by:` trailer should be placed before `Signed-off-by:` to indicate that AI assistance was used, with the human sign-off as the final confirmation.

```
<type>: <description>

Assisted-by: <name of AI tool or service>
Signed-off-by: Your Name <email@example.com>
```

Example: `Assisted-by: Claude (Anthropic)`. Use the tool's commercial name, optionally with model version.

For pull requests, include a statement like:

> This contribution was developed with assistance from [AI tool name].

### Commit Authorship

- Do not modify git user configuration; preserve the original author
- Append agent co-authorship to commit message footer

## CI Checks

The following checks run on every push and pull request:

| Check | Workflow | Purpose |
|-------|----------|---------|
| commitlint | `commitlint.yml` | Validates commit message format |
| clang-format | `reusable-call-linter.yml` | Enforces C++ code style |
| lineguard | `misc-linters.yml` | Trailing whitespace, final newline |
| codespell | `misc-linters.yml` | Catches common typos |
| CodeQL | `codeql-analysis.yml` | Security analysis |
| IWYU | `IWYU_scan.yml` | Include-what-you-use |

Build workflows: `build.yml` (core runtime), `build-extensions.yml` (plugins).

Pre-validate locally before pushing:

```bash
lineguard --config .lineguardrc -r .
```

## Common Pitfalls

- **No inline comments**: Do not add comments explaining what changed — the commit message serves that purpose.
- **Style configs are read-only**: Do not modify `.clang-format`, `.clang-tidy`, or `.lineguardrc`.
- **Platform restrictions**: Some plugins only build on certain OSes. Check `plugins/CMakeLists.txt` for guards (e.g., `wasm_bpf` and `wasmedge_process` are Linux-only).
- **spdlog `sv` suffix**: Always use `"text"sv` in spdlog format strings — omitting `sv` causes build warnings.
- **macOS vs Linux core count**: Use `sysctl -n hw.logicalcpu` on macOS, `nproc` on Linux.
