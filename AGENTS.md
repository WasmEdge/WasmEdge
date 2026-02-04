# AGENTS.md

## Code Style

- C++17 standard
- Follow LLVM coding style (enforced via `.clang-format` and `.clang-tidy`)
- Use 2-space indentation, no tabs
- Use CamelCase for classes, functions, variables, and parameters
- Use `Expect<T>` for error handling
- All files must end with a newline and have no trailing whitespace, run `lineguard` to check.
- Do not add inline comments explaining the change
- Use `sv` suffix for string literals in spdlog calls (e.g., `spdlog::info("message"sv)`)

## Building

```bash
cmake -S . -B build -DWASMEDGE_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

Common options:
- `-DWASMEDGE_BUILD_TESTS=ON` - Enable tests
- `-DWASMEDGE_BUILD_TOOLS=ON` - Enable tools (default ON)
- `-DWASMEDGE_USE_LLVM=ON` - Enable LLVM for AOT/JIT (default ON)

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

Test files are located in `test/` with structure mirroring `lib/`.

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

Common examples of the `Assisted-by:` trailer:

```
Assisted-by: GitHub Copilot
Assisted-by: Claude (Anthropic)
Assisted-by: ChatGPT (OpenAI)
Assisted-by: Cursor AI
Assisted-by: Gemini (Google)
Assisted-by: Self-hosted LLM (Llama)
```

The format is flexible. Contributors may use the tool's commercial name, optionally with model version, or a general description. If the tool is internal or subject to confidentiality, a generic description such as `Internal AI tool` is acceptable.

For pull requests, include a statement like:

> This contribution was developed with assistance from [AI tool name].

### Commit Authorship

- Do not modify git user configuration; preserve the original author
- Append agent co-authorship to commit message footer

## Adding New Files

- Source files: Add to appropriate `CMakeLists.txt` in the directory
- Test files: Add to `test/` with corresponding CMake registration
- Header files: Place in `include/` with matching namespace structure

After adding files, verify the build works.
