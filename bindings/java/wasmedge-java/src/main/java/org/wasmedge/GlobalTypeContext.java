package org.wasmedge;

import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

public class GlobalTypeContext {
    private long pointer;

    public GlobalTypeContext(ValueType valueType, Mutability mutability) {
        nativeInit(valueType.getValue(), mutability.getValue());
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

    public Mutability getMutability() {
        return Mutability.parseMutability(nativeGetMutability());
    }

    private native int nativeGetMutability();

}
