package org.wasmedge;

/**
 * Vm context, for manipulating vm.
 */
public class VmContext extends NativeResource {

    public VmContext(ConfigureContext configContext, StoreContext storeContext) {
        initNative(configContext, storeContext);
    }

    private native void initNative(ConfigureContext context, StoreContext storeContext);

    public native void close();

}
