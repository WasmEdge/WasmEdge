package org.wasmedge;

public class NativeResource {
    private long pointer;

    public NativeResource() {
        nativeInit();
    }

    public void release() {
        cleanUp();
        pointer = 0;
    }

    protected native void nativeInit();

    protected native void cleanUp();


}
