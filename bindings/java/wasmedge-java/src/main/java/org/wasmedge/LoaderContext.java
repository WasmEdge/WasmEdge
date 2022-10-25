package org.wasmedge;

public class LoaderContext {
    private long pointer;

    public LoaderContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    public native ASTModuleContext parseFromFile(String path);

    public native ASTModuleContext parseFromBuffer(byte[] buf, int bufSize);

    private native void nativeInit(ConfigureContext configureContext);

    public native void delete();
}
