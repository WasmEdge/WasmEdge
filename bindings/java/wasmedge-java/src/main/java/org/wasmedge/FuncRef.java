package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * func ref.
 */
public class FuncRef implements Value {

    private long value;

    public FuncRef() {

    }

    public FuncRef(long index) {
        this.value = index;
    }

    @Override
    public ValueType getType() {
        return ValueType.FuncRef;
    }

    public long getValue() {
        return this.value;
    }

    public void setValue(long value) {
        this.value = value;
    }

    public long getIndex() {
        return value;
    }
}
