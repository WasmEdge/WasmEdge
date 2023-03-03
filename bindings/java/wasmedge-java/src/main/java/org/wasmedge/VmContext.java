package org.wasmedge;

/**
 * Vm context, for manipulating vm.
 */
public class VmContext {
    private long pointer;

    public VmContext(ConfigureContext configContext, StoreContext storeContext) {
        initNative(configContext, storeContext);
    }

    public void release() {
        this.cleanUp();
        pointer = 0;
    }

    private native void initNative(ConfigureContext context, StoreContext storeContext);

    private native void cleanUp();

}
