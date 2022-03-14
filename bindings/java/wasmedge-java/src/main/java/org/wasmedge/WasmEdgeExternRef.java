package org.wasmedge;

import org.wasmedge.enums.ValueType;

import java.util.UUID;

public class WasmEdgeExternRef<T> implements WasmEdgeValue {
    private long pointer;
    private String value;
    public WasmEdgeExternRef(T val) {
        final String key = UUID.randomUUID().toString();
        this.value = key;
        WasmEdgeVM.addExternRef(key, val);
        nativeInit(key);
    }

    private WasmEdgeExternRef() {

    }

    private native void nativeInit(String key);

    private native String nativeGetKey();

    public String getValue() {
        return value;
    }
    public T getExtValue() {
        return (T)WasmEdgeVM.getExternRef(value);
    }

    public void setValue(String value) {
        this.value = value;
    }


    @Override
    public ValueType getType() {
        return ValueType.ExternRef;
    }

    public native void delete();

}
