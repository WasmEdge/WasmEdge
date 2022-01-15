package org.wasmedge;

public class GlobalInstanceContext {
    private GlobalTypeContext globalTypeContext;
    private WasmEdgeValue value;

    public GlobalInstanceContext(GlobalTypeContext typeCxt,
                                 WasmEdgeValue value) {
        this.globalTypeContext = typeCxt;
        this.value = value;
        nativeInit(typeCxt, value);
    }

    private native void nativeInit(GlobalTypeContext typeCxt, WasmEdgeValue value);

    public GlobalTypeContext getGlobalType() {
        return globalTypeContext;
    }

    public void setValue(WasmEdgeValue value) {
        this.value = value;
        nativeSetValue(value);
    }

    private native void nativeSetValue(WasmEdgeValue value);

    public WasmEdgeValue getValue() {
        return this.value;
    }

    public native void delete();
}
