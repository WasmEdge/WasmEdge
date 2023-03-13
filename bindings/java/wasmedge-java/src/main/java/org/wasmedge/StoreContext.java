package org.wasmedge;

import java.util.List;

/**
 * Store context for vm execution.
 */
public class StoreContext extends NativeResource {

    public StoreContext() {
        nativeInit();
    }

    private StoreContext(long pointer) {
        super(pointer);
    }

    private native void nativeInit();

    public native void close();

    public native List<String> listModule();

}
