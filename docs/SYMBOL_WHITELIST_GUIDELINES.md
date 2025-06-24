# Symbol Whitelist Change Template

When modifying `lib/api/whitelist.symbols`, use this commit message template:

```
[API] Update symbol whitelist: [brief description]

- Added: WasmEdge_NewFunction1, WasmEdge_NewFunction2
- Removed: WasmEdge_OldFunction1 (deprecated)
- Reason: [explain why these changes are necessary]

Verified with: `./.github/scripts/check_symbols.sh`

Signed-off-by: Your Name <your.email@example.com>
```

## Guidelines

1. **Always test**: Run `./.github/scripts/check_symbols.sh` before committing
2. **Document reason**: Explain why symbols are added/removed
3. **Group changes**: Add related symbols in the same commit
4. **Review thoroughly**: Symbol changes affect public API stability

## Common Scenarios

### Adding New API Functions
When adding new public API functions to WasmEdge:
1. Add the function declaration to `include/api/wasmedge/wasmedge.h`
2. Implement the function in the appropriate source file
3. Add the function name to `lib/api/whitelist.symbols`
4. Ensure the function is exported in `lib/api/libwasmedge.lds`
5. Run `./.github/scripts/check_symbols.sh` to verify

### Removing Deprecated Functions
When removing deprecated API functions:
1. Remove the function from `lib/api/whitelist.symbols`
2. Update the linker script if necessary
3. Document the removal in the commit message
4. Consider adding to a deprecation notice first

### Renaming Functions
When renaming API functions:
1. Add the new function name to the whitelist
2. Remove the old function name from the whitelist
3. Update all references in the codebase
4. Ensure both old and new are handled during transition period if needed

Remember: Symbol whitelist changes affect the public API surface and should be reviewed carefully.
