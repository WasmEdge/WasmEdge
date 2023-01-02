package org.wasmedge;

/**
 * Memory instance.
 */
public class MemoryInstanceContext {

    private long pointer;

    private MemoryTypeContext memoryTypeContext;

    private MemoryInstanceContext(long pointer) {
        this.pointer = pointer;
    }

    public MemoryInstanceContext(MemoryTypeContext memoryTypeContext) {
        this.memoryTypeContext = memoryTypeContext;
        nativeInit(memoryTypeContext);
    }


    private native void nativeInit(MemoryTypeContext memoryTypeContext);

    public native void setData(byte[] data, int offSet, int length);

    public native byte[] getData(int offSet, int length);

    public native int getPageSize();

    public native void growPage(int size);

    public native void delete();
}
