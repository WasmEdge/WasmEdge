package org.wasmedge;

import org.wasmedge.enums.ValueType;

public class WasmEdgeExternalRef implements WasmEdgeValue {
    public WasmEdgeExternalRef() {
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

}
