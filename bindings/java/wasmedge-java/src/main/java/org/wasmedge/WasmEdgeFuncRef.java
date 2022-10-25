package org.wasmedge;

import org.wasmedge.enums.ValueType;

public class WasmEdgeFuncRef implements WasmEdgeValue {
    private long value;

    public WasmEdgeFuncRef() {

    }

    public WasmEdgeFuncRef(long index) {
        this.value = index;
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

    public long getValue() {
        return this.value;
    }

    public void setValue(long value) {
        this.value = value;
    }

    public long getIndex() {
        return value;
    }
}
