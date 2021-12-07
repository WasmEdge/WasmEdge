package org.wasmedge;

import org.wasmedge.enums.ValueType;

public final class WasmEdgeI64Value implements WasmEdgeValue {
    private long value;
    public WasmEdgeI64Value(long value) {
        this.value = value;
    }

    public WasmEdgeI64Value() {

    }

    @Override
    public ValueType getType() {
        return ValueType.i64;
    }
    
    public long getValue() {
       return this.value;
    }

    public void setValue(long value) {
        this.value = value;
    }
}
