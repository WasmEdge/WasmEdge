package org.wasmedge;

import java.util.List;
import java.util.Objects;

public class ExecutorContext {
    private long pointer;

    public ExecutorContext(ConfigureContext configureContext, StatisticsContext statisticsContext) {
        nativeInit(configureContext, statisticsContext);
    }

    private native void nativeInit(ConfigureContext configureContext, StatisticsContext statisticsContext);


    public native void instantiate(StoreContext storeContext, ASTModuleContext astModuleContext);

    public void invoke(StoreContext storeContext, String funcName,
                               List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {

        if(params == null || returns == null ||
                params.stream().anyMatch(Objects::isNull) || returns.stream().anyMatch(Objects::isNull)){
            throw new IllegalArgumentException("paras or returns contain null value");
        }
        invoke(storeContext, funcName, valueListToArray(params), getValueTypeArray(params),
                valueListToArray(returns), getValueTypeArray(returns));
    }


    private int[] getValueTypeArray(List<WasmEdgeValue> values) {

        int[] types = new int[values.size()];

        for (int i = 0; i < values.size(); i++) {
            types[i] = values.get(i).getType().ordinal();
        }
        return types;
    }

    private WasmEdgeValue[] valueListToArray(List<WasmEdgeValue> values) {
        WasmEdgeValue[] valuesArray = new WasmEdgeValue[values.size()];
        values.toArray(valuesArray);
        return valuesArray;
    }

    private native void invoke(StoreContext storeContext, String funcNme, WasmEdgeValue[] params, int[] paramTypes, WasmEdgeValue[] returns,
                               int[] returnTypes);


    public void invokeRegistered(StoreContext storeContext,
                                        String modeName, String funcName,
                              List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
         invokeRegistered(storeContext, modeName, funcName, valueListToArray(params), getValueTypeArray(params),
                 valueListToArray(returns), getValueTypeArray(returns));
    }

    private native void invokeRegistered(StoreContext storeContext, String modName, String funcName,
                                         WasmEdgeValue[] params, int[] paramTypes, WasmEdgeValue[] returns, int[] returnTypes);

    public native void registerModule(StoreContext storeCxt,
                                      ASTModuleContext astCxt,
                                      String modeName);

    public native void registerImport(StoreContext storeCxt, ImportTypeContext importTypeContext);

    public native void delete();
}
