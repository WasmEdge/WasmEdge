package org.wasmedge;

public class MemoryTypeContext {
    private long pointer;
    private WasmEdgeLimit limit;

    private MemoryTypeContext(long pointer) {
        this.pointer = pointer;
    }

    public MemoryTypeContext(WasmEdgeLimit limit) {
        this.limit = limit;
        nativeInit(limit.isHasMax(), limit.getMin(), limit.getMax());
    }

    public native WasmEdgeLimit getLimit();

    private native void nativeInit(boolean hasMax, long min, long max);

    public native void delete();
}
