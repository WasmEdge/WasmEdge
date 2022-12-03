package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * i64 value.
 */
public final class I64Value implements Value {
    private long value;

    public I64Value(long value) {
        this.value = value;
    }

    public I64Value() {

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
