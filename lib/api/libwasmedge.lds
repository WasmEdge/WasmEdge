{
  global:
    WasmEdge*;
    extern "C++" {
      WasmEdge::*
    };
    extern "C++" {
      wasm::*
    };
    wasm_*;

  local:
    *;
};
