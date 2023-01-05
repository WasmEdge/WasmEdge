package org.wasmedge.enums;

/**
 * Compiler optimization level enums.
 */
public enum CompilerOptimizationLevel {
    /// Disable as many optimizations as possible.
    WasmEdge_CompilerOptimizationLevel_O0,
    /// Optimize quickly without destroying debuggability.
    WasmEdge_CompilerOptimizationLevel_O1,
    /// Optimize for fast execution as much as possible without triggering
    /// significant incremental compile time or code size growth.
    WasmEdge_CompilerOptimizationLevel_O2,
    /// Optimize for fast execution as much as possible.
    WasmEdge_CompilerOptimizationLevel_O3,
    /// Optimize for small code size as much as possible without triggering
    /// significant incremental compile time or execution time slowdowns.
    WasmEdge_CompilerOptimizationLevel_Os,
    /// Optimize for small code size as much as possible.
    WasmEdge_CompilerOptimizationLevel_Oz
}
