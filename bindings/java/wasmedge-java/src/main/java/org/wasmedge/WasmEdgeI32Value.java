package org.wasmedge;

public final class WasmEdgeI32Value implements WasmEdgeValue {
    private int value;

    public WasmEdgeI32Value() {

    }
    public WasmEdgeI32Value(int value) {
        this.value = value;
    }

    @Override
    public ValueType getType() {
        return ValueType.i32;
    }

    public int getValue() {
       return this.value;
    }

    public void setValue(int value) {
        this.value = value;
    }
}
