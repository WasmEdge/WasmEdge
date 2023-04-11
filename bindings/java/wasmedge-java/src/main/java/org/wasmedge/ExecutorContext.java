package org.wasmedge;

import java.util.List;

/**
 * Executor context, for executing wasm modules.
 */
public class ExecutorContext extends NativeResource {

    public ExecutorContext(ConfigureContext configureContext, StatisticsContext statisticsContext) {
        super();
        nativeInit(configureContext, statisticsContext);
    }

    private native void nativeInit(ConfigureContext configureContext,
                                   StatisticsContext statisticsContext);


    public native ModuleInstanceContext instantiate(StoreContext storeContext,
                                                    AstModuleContext astModuleContext);

    public native void invoke(FunctionInstanceContext functionInstanceContext, List<Value> params,
                              List<Value> returns);

    public native ModuleInstanceContext register(StoreContext storeCxt, AstModuleContext astCxt,
                                                 String modeName);

    public native void registerImport(StoreContext storeCxt,
                                      ModuleInstanceContext moduleInstanceContext);

    public native void close();
}
