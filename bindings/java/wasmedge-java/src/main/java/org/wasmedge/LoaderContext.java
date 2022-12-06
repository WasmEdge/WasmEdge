package org.wasmedge;

/**
 * Load context.
 */
public class LoaderContext {
    private long pointer;

    public LoaderContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    public native AstModuleContext parseFromFile(String path);

    public native AstModuleContext parseFromBuffer(byte[] buf, int bufSize);

    private native void nativeInit(ConfigureContext configureContext);

    public native void delete();
}
