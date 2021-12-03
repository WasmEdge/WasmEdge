package org.wasmedge;

public class MemoryTypeContext {
    private long pointer;

    public MemoryTypeContext(WasmEdgeLimit limit) {
        nativeInit(limit);
    }

    private native void nativeInit(WasmEdgeLimit limit);

    public native void delete();
}
