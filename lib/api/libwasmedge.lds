{
  global:
    # Only expose the C API symbols from whitelist.symbols
    WasmEdge_*;

  local:
    # Hide all other symbols, including C++ symbols
    *;
};
