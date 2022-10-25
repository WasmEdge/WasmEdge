package org.wasmedge;

import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.WasmEdgeMutability;

public class GlobalTypeContext {
    private long pointer;

    public GlobalTypeContext(ValueType valueType, WasmEdgeMutability wasmEdgeMutability) {
        nativeInit(valueType.getValue(), wasmEdgeMutability.getValue());
    }

    private GlobalTypeContext(long pointer) {
        this.pointer = pointer;
    }

    private native void nativeInit(int valueType, int wasmEdgeMutability);

    public native void delete();

    public ValueType getValueType() {
        return ValueType.parseType(nativeGetValueType());
    }

    private native int nativeGetValueType();

    public WasmEdgeMutability getMutability() {
        return WasmEdgeMutability.parseMutability(nativeGetMutability());
    }

    private native int nativeGetMutability();

}
