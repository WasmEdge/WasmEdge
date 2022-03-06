package org.wasmedge;

import org.wasmedge.enums.ValueType;

public class WasmEdgeFunctionRef implements WasmEdgeValue {
    private long value;
    public WasmEdgeFunctionRef(long index) {
        this.value = index;
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

    public long getValue() {
        return this.value;
    }

    public long getIndex() {
        return value;
    }
}
