package org.wasmedge;

import java.util.List;

public class ExecutorContext {
    private long pointer;

    public ExecutorContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    private native void nativeInit(ConfigureContext configureContext);

    public native void instantiate();

    public native void instantiate(StoreContext storeContext, ASTModuleContext astModuleContext);

    public native void invoke(StoreContext storeContext, String funName,
                               List<WasmEdgeValue> params, List<WasmEdgeValue> returns);


    public native void invokeRegistered(StoreContext storeContext,
                                        String modeName, String funName,
                              List<WasmEdgeValue> params, List<WasmEdgeValue> returns);

    public native void registerModule(StoreContext storeCxt,
                                      ASTModuleContext astCxt,
                                      String modeName);

    public native void registerImport(StoreContext storeCxt, ImportTypeContext importTypeContext);

    public native void delete();
}
