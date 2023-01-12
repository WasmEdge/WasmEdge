package org.wasmedge.enums;

/**
 * Compiler output format enum.
 */
public enum CompilerOutputFormat {
    /// Native dynamic library format.
    WasmEdge_CompilerOutputFormat_Native,
    /// WebAssembly with AOT compiled codes in custom section.
    WasmEdge_CompilerOutputFormat_Wasm
}
