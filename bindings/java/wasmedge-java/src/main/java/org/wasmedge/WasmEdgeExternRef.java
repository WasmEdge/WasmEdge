package org.wasmedge;

import org.wasmedge.enums.ValueType;

import java.util.UUID;

public class WasmEdgeExternRef implements WasmEdgeValue {
    private String value;
    private long pointer;
    public WasmEdgeExternRef(Object val) {
        String key = UUID.randomUUID().toString();
        WasmEdgeVM.addExternRef(key, val);
        nativeInit(key);
    }

    private void WasmEdgeFunctionRef() {

    }

    private native void nativeInit(String key);

    public Object getExternRefVal() {
        return WasmEdgeVM.getExternRef(value);
    }

    @Override
    public ValueType getType() {
        return ValueType.ExternRef;
    }

}
