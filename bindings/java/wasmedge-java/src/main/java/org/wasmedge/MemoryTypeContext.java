package org.wasmedge;

public class MemoryTypeContext {
    private long pointer;
    private WasmEdgeLimit limit;

    private MemoryTypeContext(long pointer) {
        this.pointer = pointer;
    }
    public MemoryTypeContext(WasmEdgeLimit limit) {
        this.limit = limit;
        nativeInit(limit);
    }

    public native WasmEdgeLimit getLimit();

    private native void nativeInit(WasmEdgeLimit limit);

    public native void delete();
}
