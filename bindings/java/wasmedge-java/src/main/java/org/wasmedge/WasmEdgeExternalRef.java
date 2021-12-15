package org.wasmedge;

import org.wasmedge.enums.ValueType;

public class WasmEdgeExternalRef implements WasmEdgeValue {
    public WasmEdgeExternalRef(TableTypeContext tab) {
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

}
