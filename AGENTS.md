# AGENTS.md

## Code Style

- C++17 standard
- Follow LLVM coding style (enforced via `.clang-format`)
- Use 2-space indentation, no tabs
- Use CamelCase for classes, functions, variables, and parameters
- Use `Expect<T>` for error handling
- All files must end with a newline and have no trailing whitespace
- Do not add inline comments explaining the change

## Building

```bash
cmake -S . -B build -DWASMEDGE_BUILD_TESTS=ON
cmake --build build -j$(nproc)
```

Common options:
- `-DWASMEDGE_BUILD_TESTS=ON` - Enable tests
- `-DWASMEDGE_BUILD_TOOLS=ON` - Enable tools (default ON)
- `-DWASMEDGE_USE_LLVM=ON` - Enable LLVM for AOT/JIT (default ON)

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

## Commit Authorship

- Do not modify git user configuration; preserve the original author
- Append agent co-authorship to commit message footer

## Adding New Files

- Source files: Add to appropriate `CMakeLists.txt` in the directory
- Test files: Add to `test/` with corresponding CMake registration
- Header files: Place in `include/` with matching namespace structure

After adding files, verify the build works.
