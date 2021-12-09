package org.wasmedge;

import org.wasmedge.enums.ValueType;

public class WasmEdgeFunctionRef implements WasmEdgeValue {
    int index;
    public WasmEdgeFunctionRef(int index) {
        this.index = index;
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

    public int getIndex() {
        return index;
    }
}
