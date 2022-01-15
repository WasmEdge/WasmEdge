package org.wasmedge;

public class MemoryTypeContext {
    private long pointer;
    private WasmEdgeLimit limit;

    public MemoryTypeContext(WasmEdgeLimit limit) {
        this.limit = limit;
        nativeInit(limit);
    }

    public WasmEdgeLimit getLimit() {
        return limit;
    }

    private native void nativeInit(WasmEdgeLimit limit);

    public native void delete();
}
