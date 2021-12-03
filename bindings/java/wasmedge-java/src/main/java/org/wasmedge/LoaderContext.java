package org.wasmedge;

public class LoaderContext {
    private long pointer;

    public LoaderContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    private native void nativeInit(ConfigureContext configureContext);

}
