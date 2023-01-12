package org.wasmedge;

import java.util.List;

/**
 * Store context for vm execution.
 */
public class StoreContext {
    private long pointer = 0;

    public StoreContext() {
        nativeInit();
    }

    private StoreContext(long pointer) {
        this.pointer = pointer;
    }

    public void destroy() {
        delete();
        pointer = 0;
    }

    private native void nativeInit();

    private native void delete();

    public native List<String> listModule();

}
