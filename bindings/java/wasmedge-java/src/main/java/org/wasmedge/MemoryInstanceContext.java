package org.wasmedge;

/**
 * Memory instance.
 */
public class MemoryInstanceContext extends NativeResource {


    private MemoryTypeContext memoryTypeContext;

    private MemoryInstanceContext(long pointer) {
        super(pointer);
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

    public native void close();
}
