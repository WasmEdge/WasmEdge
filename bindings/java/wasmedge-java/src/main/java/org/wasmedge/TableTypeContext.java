package org.wasmedge;

public class TableTypeContext {

    private long pointer;
    private WasmEdgeLimit limit;

    public TableTypeContext(WasmEdgeLimit limit) {
        this.limit = limit;
        nativeInit(this.limit);
    }

    private native void nativeInit(WasmEdgeLimit limit);

    public native void delete();

}
