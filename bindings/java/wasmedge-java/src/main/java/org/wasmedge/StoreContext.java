package org.wasmedge;

public class StoreContext {
    private long pointer = 0;

    public StoreContext() {
        nativeInit();
    }

    public void release() {
        cleanUp();
        pointer = 0;
    }

    private native void nativeInit();
    private native void cleanUp();


}
