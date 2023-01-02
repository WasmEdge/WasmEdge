package org.wasmedge;

import java.util.UUID;
import org.wasmedge.enums.ValueType;

/**
 * External ref, used to pass external ref from java to wasm.
 *
 * @param <T> Type of external ref
 */
public class ExternRef<T> implements Value {
    private long pointer;
    private String value;

    /**
     * Create an external ref by passing value.
     *
     * @param val the value to be referred.
     */
    public ExternRef(T val) {
        final String key = UUID.randomUUID().toString();
        this.value = key;
        WasmEdgeVm.addExternRef(key, val);
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
        return (T) WasmEdgeVm.getExternRef(value);
    }

    @Override
    public ValueType getType() {
        return ValueType.ExternRef;
    }

    public native void delete();

}
