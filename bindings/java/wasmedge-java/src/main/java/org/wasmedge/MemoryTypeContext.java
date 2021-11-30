package org.wasmedge;

public class MemoryTypeContext {
    private long pointer;
    private WasmEdgeLimit limit;

    public MemoryTypeContext(WasmEdgeLimit limit) {
        this.limit = limit;

    }

    private native void nativeInit(WasmEdgeLimit limit);

    public native void delete();
}
