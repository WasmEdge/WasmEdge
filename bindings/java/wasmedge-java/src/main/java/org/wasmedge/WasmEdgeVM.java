package org.wasmedge;

import org.wasmedge.enums.HostRegistration;

import java.util.List;

public class WasmEdgeVM {
    private long pointer;
    private ConfigureContext configureContext;
    private StoreContext storeContext;

    public WasmEdgeVM(ConfigureContext configureContext, StoreContext storeContext) {
        this.configureContext = configureContext;
        this.storeContext = storeContext;
        nativeInit(this.configureContext, this.storeContext);
    }

    private native void nativeInit(ConfigureContext configureContext, StoreContext storeContext);

    private native void runWasmFromFile(String file,
                                        String funcName,
                                        WasmEdgeValue[] params,
                                        int paramSize,
                                        int[] paramTypes,
                                        WasmEdgeValue[] returns,
                                        int returnSize,
                                        int[] returnTypes
    );

    /**
     * Run a wasm file.
     *
     * @param file     file path.
     * @param funcName function name to run.
     * @param params   params for the function.
     * @param returns  return values.
     */
    public void runWasmFromFile(String file,
                                String funcName,
                                List<WasmEdgeValue> params,
                                List<WasmEdgeValue> returns) {
        WasmEdgeValue[] paramsArray = valueListToArray(params);
        int[] paramTypes = getValueTypeArray(params);

        WasmEdgeValue[] returnsArray = valueListToArray(returns);
        int[] returnTypes = getValueTypeArray(returns);

        runWasmFromFile(file, funcName, paramsArray, params.size(), paramTypes,
                returnsArray, returns.size(), returnTypes);
    }

    public native void runWasmFromBuffer(byte[] buffer, String funcName, List<WasmEdgeValue> parasm, List<WasmEdgeValue> returns);

    public native void runWasmFromASTModule(ASTModuleContext astModuleContext, String funcName, List<WasmEdgeValue> params, List<WasmEdgeValue> returns);

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

    public native void loadWasmFromFile(String filePath);

    public native void loadWasmFromBuffer(byte[] buffer);

    public native void loadWasmFromASTModule(ASTModuleContext astModuleContext);

    public native void validate();

    public native void instantiate();

    public void execute(String funcName, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
        WasmEdgeValue[] paramsArray = valueListToArray(params);
        int[] paramTypes = getValueTypeArray(params);

        WasmEdgeValue[] returnsArray = valueListToArray(returns);
        int[] returnTypes = getValueTypeArray(returns);

        execute(funcName, paramsArray, params.size(), paramTypes, returnsArray, returns.size(), returnTypes);
    }

    public native void execute(String funcName, WasmEdgeValue[] params,
                         int paramSize,
                         int[] paramTypes,
                         WasmEdgeValue[] returns,
                         int returnSize,
                         int[] returnTypes);


    public void destroy() {
        if(configureContext != null) {
            configureContext.destroy();
        }

        if(storeContext != null) {
            storeContext.destroy();
        }
        delete();
        this.pointer = 0;
    }

    public native void registerModuleFromFile(String modName, String fileName);

    public native void registerModuleFromBuffer(String moduleName, byte[] buffer);

    public native void registerModuleFromImport(ImportObjectContext importObjectContext);

    public native void registerModuleFromASTModule(String moduleName, ASTModuleContext astModuleContext);

    public native void executeRegistered(String modName, String funcName, List<WasmEdgeValue> params,
                                         List<WasmEdgeValue> returns);

    public native List<FunctionTypeContext> getFunctionList();

    public native FunctionTypeContext getFunctionType(String funcName);

    public native ImportObjectContext getImportModuleContext(HostRegistration reg);
    public native StoreContext getStoreContext();
    public native StatisticsContext getStatisticsContext();
    public native FunctionTypeContext getFunctionTypeRegistered(String moduleName,
                                                  String funcName);

    public native void cleanUp();
    private native void delete();
}
