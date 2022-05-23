package org.wasmedge;

public class VMContext {
    private long pointer;

    public VMContext(ConfigureContext configContext, StoreContext storeContext) {
        initNative(configContext, storeContext);
    }

    public void release() {
        this.cleanUp();
        pointer = 0;
    }

    private native void initNative(ConfigureContext context, StoreContext storeContext);

    private native void cleanUp();

}
