package org.wasmedge;

public final class WasmEdgeF64Value implements WasmEdgeValue {
    private double value;
    public WasmEdgeF64Value(double value) {
        this.value = value;
    }

    public WasmEdgeF64Value() {

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
