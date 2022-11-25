package org.wasmedge;

import org.wasmedge.enums.ValueType;

public final class I32Value implements Value {
    private int value;

    public I32Value() {

    }

    public I32Value(int value) {
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
