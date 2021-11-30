package org.wasmedge;

public class GlobalTypeContext {
    private long pointer;
    public GlobalTypeContext() {
        nativeInit();
    }

    private native void nativeInit();
}
