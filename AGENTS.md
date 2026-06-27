## Project Overview

WasmEdge is a C++17 WebAssembly runtime. It loads, validates, and executes
core `.wasm` modules, with experimental Component Model support wired through
the loader, validator, executor, VM, and `--enable-component`.

Key directories:

| Path | Purpose |
|------|---------|
| `include/` | Public headers, mostly mirroring `lib/` plus header-only runtime/AST APIs |
| `lib/` | Core runtime implementations: loader, validator, executor, VM, C API, plugins |
| `plugins/` | Optional external host-function and component plugins |
| `test/` | Google Test suites, spec tests, component tests, and plugin tests |
| `tools/` | CLI binaries (`wasmedge`, `wasmedgec`) and fuzz/RPC tools |
| `examples/` | Sample C API, C++ embedding, plugin, JS, Wasm, and Android usage |

Entry points: `include/vm/vm.h` (runtime), `include/plugin/plugin.h` (plugin API).

### Core Runtime (`include/` + `lib/`)

The `include/` and `lib/` directories mostly mirror each other, but
`include/` also carries public AST/runtime/component headers and `lib/` has
implementation-only areas such as `lib/wasi_nn_rpc/`.

| Module | Purpose |
|--------|---------|
| `loader/` | Parses core modules and components into ASTs |
| `validator/` | Validates core modules and in-progress Component Model support |
| `executor/` | Interprets instructions and handles instantiation/invocation paths |
| `vm/` | Orchestrates the full pipeline (load → validate → instantiate → execute) |
| `aot/` + `llvm/` | AOT/JIT compilation via LLVM |
| `ast/` | AST node definitions for Wasm modules and components |
| `common/` | Shared types: `ErrCode`, `Expected<T>`, `Span`, configuration enums |
| `host/` | WASI and host function implementations |
| `api/` | C API surface (`include/api/wasmedge/wasmedge.h`) |
| `runtime/` | Runtime instances (module, component, function, memory, table, global) |
| `plugin/` | Plugin loading infrastructure and built-in `wasi_logging` plugin |
| `system/` | OS abstraction (mmap, signals) |
| `driver/` | CLI drivers for `wasmedge`, `wasmedgec`, fuzz tools, and WASI-NN RPC server |
| `po/` | Program options parsing |
| `experimental/` | Public experimental API headers |
| `wasi_nn_rpc/` | WASI-NN RPC support implementation |

## Prerequisites

- CMake 3.18+
- C++17 compiler (GCC 11+, Clang 13+, or MSVC 19.29+)
- Optional: LLVM/LLD development packages for AOT/JIT when
  `WASMEDGE_USE_LLVM=ON`; use the version configured by the relevant CI or
  Docker image for the target platform.

## Code Style

- C++17 standard
- Follow LLVM coding style (enforced via `.clang-format` and `.clang-tidy`)
- 2-space indentation, no tabs; UTF-8 encoding; LF line endings
- CamelCase for classes, functions, variables, and parameters
- All files must end with a newline and have no trailing whitespace; run `lineguard` to check
- Do not add inline comments explaining the change
- Use the `sv` suffix for string literals in spdlog calls (e.g., `spdlog::info("message"sv)`)

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
- Propagate errors with the `EXPECTED_TRY(expr)` macro (returns early on failure)

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
| `WASMEDGE_BUILD_COVERAGE` | OFF | Generate coverage report; requires tests |
| `WASMEDGE_BUILD_TOOLS` | ON | Build CLI tools |
| `WASMEDGE_BUILD_FUZZING` | OFF | Build fuzz tools; mutually exclusive with tests and normal tools |
| `WASMEDGE_BUILD_PLUGINS` | ON | Build enabled plugins |
| `WASMEDGE_BUILD_EXAMPLE` | OFF | Build `examples/plugin/get-string` |
| `WASMEDGE_BUILD_WASI_NN_RPC` | OFF | Build WASI-NN RPC support/server; requires shared library |
| `WASMEDGE_USE_LLVM` | ON | LLVM-based AOT/JIT compilation |
| `WASMEDGE_BUILD_SHARED_LIB` | ON | Build shared library |
| `WASMEDGE_BUILD_STATIC_LIB` | OFF | Build static library |
| `WASMEDGE_USE_CXX11_ABI` | ON | Use the libstdc++ C++11 ABI |
| `WASMEDGE_FORCE_DISABLE_LTO` | OFF | Disable link-time optimization in release-style builds |
| `WASMEDGE_LINK_LLVM_STATIC` | OFF | Link LLVM statically |
| `WASMEDGE_LINK_TOOLS_STATIC` | OFF | Link tools statically; forces static library and static LLVM |
| `WASMEDGE_ENABLE_UB_SANITIZER` | OFF | Enable undefined behavior sanitizer |
| `WASMEDGE_DISABLE_LIBTINFO` | OFF | Disable linking against `libtinfo` when linking LLVM |

### Plugin CMake Flags

External plugin switches live in the root `CMakeLists.txt`. Most are boolean
`WASMEDGE_PLUGIN_<NAME>` options and default to OFF:

`WASI_CRYPTO`, `WASI_HTTP`, `WASI_POLL`, `WASM_BPF`, `FFMPEG`, `IMAGE`, `OCR`,
`OPENCVMINI`, `PROCESS`, `STABLEDIFFUSION`, `TENSORFLOW`, `TENSORFLOWLITE`,
`ZLIB`

Current special cases:

- `WASMEDGE_PLUGIN_WASI_LOGGING` defaults to ON, but the external plugin option
  is currently a placeholder; `wasi_logging` is registered as a built-in plugin
  from `lib/plugin/wasi_logging/`.
- `WASMEDGE_PLUGIN_WASI_NN_BACKEND` is a string/list cache variable, not a
  boolean option. Current backend values used by CI include `GGML`, `OpenVINO`,
  `OpenVINOGenAI`, `Piper`, `PyTorch`, `TensorFlowLite`, `Whisper`, `MLX`,
  `ChatTTS`, and `BitNet`.
- WASI-NN has backend tuning options such as
  `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_NATIVE`,
  `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_BLAS`,
  `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_CUBLAS`,
  `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_METAL`,
  `WASMEDGE_PLUGIN_WASI_NN_GGML_LLAMA_HIP`,
  `WASMEDGE_PLUGIN_WASI_NN_WHISPER_METAL`,
  `WASMEDGE_PLUGIN_WASI_NN_WHISPER_CUDA`,
  `WASMEDGE_PLUGIN_WASI_NN_BITNET_ARM_TL1`, and
  `WASMEDGE_PLUGIN_WASI_NN_BITNET_X86_TL2`.
- Stable Diffusion has `WASMEDGE_PLUGIN_STABLEDIFFUSION_CUDA`,
  `WASMEDGE_PLUGIN_STABLEDIFFUSION_METAL`, and
  `WASMEDGE_PLUGIN_STABLEDIFFUSION_OPENMP`.
- `wasm_bpf` also has
  `WASMEDGE_PLUGIN_WASM_BPF_BUILD_LIBBPF_WITH_PKG_CONF`.
- `WASMEDGE_FORCE_DOWNLOAD_SIMDJSON` controls the simdjson dependency for
  WASI-NN.
- `WASMEDGE_PLUGIN_LLMC` is kept as a deprecated compatibility option and is
  described as removed in the root `CMakeLists.txt`.

Build with a specific plugin enabled:

```bash
cmake -S . -B build -DWASMEDGE_PLUGIN_IMAGE=ON -DWASMEDGE_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

Build WASI-NN with a backend:

```bash
cmake -S . -B build -DWASMEDGE_PLUGIN_WASI_NN_BACKEND=GGML -DWASMEDGE_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

## Build Artifacts

After building, key paths are:

- Binary: `build/tools/wasmedge/wasmedge`
- AOT compiler: `build/tools/wasmedge/wasmedgec` when `WASMEDGE_USE_LLVM=ON`
- WASI-NN RPC server: `build/tools/wasmedge/wasi_nn_rpcserver` when
  `WASMEDGE_BUILD_WASI_NN_RPC=ON`
- Fuzz tools: `build/tools/fuzz/wasmedge-fuzztool` and
  `build/tools/fuzz/wasmedge-fuzzpo` when `WASMEDGE_BUILD_FUZZING=ON`
- Libraries: `build/lib/api/`
- External plugins: `build/plugins/<plugin-dir>/`
- Built-in `wasi_logging`: `build/lib/plugin/wasi_logging/`

To run with plugins:

```bash
WASMEDGE_PLUGIN_PATH=build/plugins ./build/tools/wasmedge/wasmedge <wasm-file>
```

`WASMEDGE_PLUGIN_PATH` can point at `build/plugins` or a plugin-specific
subdirectory; the loader recursively scans plugin directories.

## Testing

Tests use the Google Test framework:

```bash
cmake --build build --target <test-name-regex>
```

Run a specific test:

```bash
cd build && ctest -R <test-name-regex>
```

Test files are located in `test/`, with a structure mirroring `lib/`. Plugin tests live in `test/plugins/`.

### Test Case Guidelines

- **Avoid adding new test targets**: Do not casually register a new CMake test executable (`wasmedge_add_test` / `add_test`) for new cases. Prefer adding `TEST(...)` cases to an existing test target whose scope matches (e.g. add loader/serializer cases to `test/loader`), so the suite stays consolidated and CI does not gain extra binaries to build and run.
- Only create a new test target when the work introduces a genuinely new component or test category with no suitable existing home; when you do, justify it and register it in the directory's `CMakeLists.txt` following the surrounding pattern.

## Plugin System

Plugins add host functions that Wasm modules or components can call. See
`plugins/wasmedge_image/` for a simple external module plugin example, and
`lib/plugin/wasi_logging/` for the current built-in plugin.

### File Structure

A typical module-based plugin uses prefixed files like `image_base.h`, or plain
names like `base.h` in smaller plugins:

- `*_base.h` / `base.h` — CRTP host function base class
- `*_env.h` / `env.h` and `*_env.cpp` / `env.cpp` — environment state + plugin descriptor
- `*_func.h` / `func.h` and `*_func.cpp` / `func.cpp` — host function implementations
- `*_module.h` / `module.h` and `*_module.cpp` / `module.cpp` — module or component instance that registers host functions
- `CMakeLists.txt` — build configuration

### Key Patterns

- **CRTP host function base**: Each plugin defines a `Func<T>` class inheriting `Runtime::HostFunction<T>` that holds a reference to the plugin's environment. See `plugins/wasmedge_image/image_base.h`.
- **Plugin descriptor**: A `PluginDescriptor` struct in `env.cpp` declares the plugin name, version, and module factory. The plugin must call `EXPORT_GET_DESCRIPTOR(Descriptor)`. See `plugins/wasmedge_image/image_env.cpp`.
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
target_compile_options(wasmedgePluginMyPlugin PUBLIC -DWASMEDGE_PLUGIN)
target_include_directories(wasmedgePluginMyPlugin
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginMyPlugin PRIVATE wasmedgeCAPI)
else()
  target_link_libraries(wasmedgePluginMyPlugin PRIVATE wasmedge_shared)
endif()
```

## Adding New Files

- Source files: Add them to the appropriate `CMakeLists.txt` in the directory
- Test files: Add them to `test/` with the corresponding CMake registration
- Header files: Place them in `include/` with the matching namespace structure
- New external plugins require an option in the root `CMakeLists.txt` and a
  conditional entry in `plugins/CMakeLists.txt`
- Built-in plugins under `lib/plugin/` also need `lib/plugin/CMakeLists.txt`
  updates

After adding files, verify the build works.

## Git Commits

Use the Conventional Commits format with DCO sign-off:

```text
<type>: <description>

Signed-off-by: Your Name <email@example.com>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`, `ci`

Rules:

- Header: 100 characters maximum
- Each logical change gets its own commit
- Sign-off required: `git commit -s`
- Use `commitlint` for validation

### AI Assistance Disclosure

Use the `Assisted-by:` trailer for the AI assistance disclosure.

The `Assisted-by:` trailer should be placed before `Signed-off-by:` to indicate that AI assistance was used, with the human sign-off as the final confirmation.

```text
<type>: <description>

Assisted-by: <name of AI tool or service>
Signed-off-by: Your Name <email@example.com>
```

Example: `Assisted-by: Claude (Anthropic)`. Use the tool's commercial name and, optionally, its model version.

For pull requests, include a statement like:

> This contribution was developed with assistance from [AI tool name].

### Commit Authorship

- Do not modify the git user configuration; preserve the original author
- Append agent co-authorship to the commit message footer

## CI Checks

The following checks run on relevant pushes and pull requests; several workflows
have path filters:

| Check | Workflow | Purpose |
|-------|----------|---------|
| commitlint | `commitlint.yml` | Validates PR commit messages and PR title |
| clang-format | `reusable-call-linter.yml` | Enforces C++ code style when invoked by other workflows |
| lineguard | `misc-linters.yml` | Trailing whitespace, final newline |
| codespell | `misc-linters.yml` | Catches common typos |
| CodeQL | `codeql-analysis.yml` | Security analysis |
| IWYU | `IWYU_scan.yml` | Include-what-you-use |
| Infer | `static-code-analysis.yml` | Static code analysis |

Build workflows: `build.yml` (Core), `build-extensions.yml` (Extensions), plus
targeted platform, installer, release, Docker, and WASI testsuite workflows.

Pre-validate locally before pushing:

```bash
lineguard --config .lineguardrc -r .
```

## Common Pitfalls

- **No inline comments**: Do not add comments explaining what changed — the commit message serves that purpose.
- **Style configs are read-only**: Do not modify `.clang-format`, `.clang-tidy`, or `.lineguardrc`.
- **Platform restrictions**: Some plugins only build on certain operating systems. Check `plugins/CMakeLists.txt` for guards (e.g., `wasm_bpf` and `wasmedge_process` are Linux-only).
- **spdlog `sv` suffix**: Always use `"text"sv` in spdlog format strings — omitting `sv` causes build warnings.
- **macOS vs Linux core count**: Use `sysctl -n hw.logicalcpu` on macOS, `nproc` on Linux.
