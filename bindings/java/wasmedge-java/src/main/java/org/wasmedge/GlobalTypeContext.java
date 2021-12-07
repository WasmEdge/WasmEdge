package org.wasmedge;

import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.WasmEdgeMutability;

public class GlobalTypeContext {
    private long pointer;

    public GlobalTypeContext(ValueType valueType, WasmEdgeMutability wasmEdgeMutability) {
        nativeInit(valueType.ordinal(), wasmEdgeMutability.ordinal());
    }

    private native void nativeInit(int valueType, int wasmEdgeMutability);

    public native void delete();
}
