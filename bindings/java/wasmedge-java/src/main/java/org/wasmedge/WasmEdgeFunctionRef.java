package org.wasmedge;

import org.wasmedge.enums.ValueType;

public class WasmEdgeFunctionRef implements WasmEdgeValue {
    private int value;
    public WasmEdgeFunctionRef(int index) {
        this.value = index;
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

    public int getIndex() {
        return value;
    }
}
