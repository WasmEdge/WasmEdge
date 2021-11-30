package org.wasmedge;

import org.wasmedge.enums.WasmEdgeMutability;

public class GlobalTypeContext {
    private long pointer;
    public GlobalTypeContext(ValueType valueType, WasmEdgeMutability wasmEdgeMutability) {
        nativeInit(valueType, wasmEdgeMutability);
    }

    private native void nativeInit(ValueType valueType, WasmEdgeMutability wasmEdgeMutability);

    public native void delete();
}
