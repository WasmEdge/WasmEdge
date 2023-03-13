package org.wasmedge;

/**
 * Memory type.
 */
public class MemoryTypeContext extends NativeResource {
    private Limit limit;

    private MemoryTypeContext(long pointer) {
        super(pointer);
    }

    public MemoryTypeContext(Limit limit) {
        this.limit = limit;
        nativeInit(limit.isHasMax(), limit.getMin(), limit.getMax());
    }

    public native Limit getLimit();

    private native void nativeInit(boolean hasMax, long min, long max);

    public native void close();
}
