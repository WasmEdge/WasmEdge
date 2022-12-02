package org.wasmedge;

import org.wasmedge.enums.ValueType;

import java.util.UUID;

public class ExternRef<T> implements Value {
    private long pointer;
    private String value;

    public ExternRef(T val) {
        final String key = UUID.randomUUID().toString();
        this.value = key;
        WasmEdgeVM.addExternRef(key, val);
        nativeInit(key);
    }

    private ExternRef() {

    }

    private native void nativeInit(String key);

    private native String nativeGetKey();

    public String getValue() {
        return value;
    }

    public void setValue(String value) {
        this.value = value;
    }

    public T getExtValue() {
        return (T) WasmEdgeVM.getExternRef(value);
    }

    @Override
    public ValueType getType() {
        return ValueType.ExternRef;
    }

    public native void delete();

}
