package org.wasmedge;

import org.wasmedge.enums.ValueType;

public final class WasmEdgeF32Value implements WasmEdgeValue {
    private float value;

    public WasmEdgeF32Value() {

    }

    public WasmEdgeF32Value(float value) {
        this.value = value;
    }

    @Override
    public ValueType getType() {
        return ValueType.f32;
    }

    public float getValue() {
        return this.value;
    }

    public void setValue(float value) {
        this.value = value;
    }
}
