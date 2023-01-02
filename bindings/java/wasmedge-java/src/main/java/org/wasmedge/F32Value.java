package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * f32 value.
 */
public final class F32Value implements Value {
    private float value;

    public F32Value() {

    }

    public F32Value(float value) {
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
