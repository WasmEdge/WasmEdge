package org.wasmedge;

public class MemoryInstanceContext {
    public MemoryInstanceContext(MemoryTypeContext memoryTypeContext) {

    }

    public native void setData(byte[] data, int offSet, int length);

    public native void getData(byte[] buf, int offSet, int length);

    public native int getPageSize();

    public native void growPage(int size);

    public native void delete();
}
