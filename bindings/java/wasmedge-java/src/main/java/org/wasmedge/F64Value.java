package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * f4 value.
 */
public final class F64Value implements Value {
    private double value;

    public F64Value(double value) {
        this.value = value;
    }

    public F64Value() {

    }

    @Override
    public ValueType getType() {
        return ValueType.f64;
    }

    public double getValue() {
        return this.value;
    }

    public void setValue(double value) {
        this.value = value;
    }
}
