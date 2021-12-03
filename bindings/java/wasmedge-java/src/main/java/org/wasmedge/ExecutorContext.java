package org.wasmedge;

public class ExecutorContext {
    private long pointer;

    public ExecutorContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    private native void nativeInit(ConfigureContext configureContext);

    public native void registerModule(StoreContext storeCxt,
                                      ASTModuleContext astCxt,
                                      String modeName);

    public native void registerImport(StoreContext storeCxt, ImportTypeContext importTypeContext);
}
