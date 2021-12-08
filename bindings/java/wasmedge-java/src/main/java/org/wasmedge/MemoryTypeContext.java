package org.wasmedge;

public class MemoryTypeContext {
    private long pointer;

    public MemoryTypeContext(WasmEdgeLimit limit) {
        nativeInit(limit);
    }

    public native WasmEdgeLimit getLimit();

    private native void nativeInit(WasmEdgeLimit limit);

    public native void delete();
}
