WASMEDGE_0.16 {
  global:
    WasmEdge*;
    extern "C++" {
      WasmEdge::*
    };

  local:
    # Internal names of the 0.16 backward-compat shims (see
    # lib/api/wasmedge_compat.cpp).
    # They need default visibility so `.symver` can export their `@WASMEDGE_0.16`
    # aliases, but their own names match the `WasmEdge*` pattern above; listing
    # them here (an exact match outranks the wildcard) keeps them unexported.
    WasmEdge_LimitIsEqual_Compat_016;
    WasmEdge_MemoryTypeCreate_Compat_016;
    WasmEdge_MemoryTypeGetLimit_Compat_016;
    WasmEdge_TableTypeCreate_Compat_016;
    WasmEdge_TableTypeGetLimit_Compat_016;
    *;
};

WASMEDGE_0.17 {
  global:
    WasmEdge_LimitIsEqual;
    WasmEdge_MemoryTypeCreate;
    WasmEdge_MemoryTypeGetLimit;
    WasmEdge_TableTypeCreate;
    WasmEdge_TableTypeGetLimit;
} WASMEDGE_0.16;
