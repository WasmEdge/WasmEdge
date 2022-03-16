package org.wasmedge;

import org.wasmedge.enums.HostRegistration;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class WasmEdgeVM {
    private static final Map<String, Object> externRefMap = new HashMap<>();
    private long pointer;
    private ConfigureContext configureContext;
    private StoreContext storeContext;


    public WasmEdgeVM(ConfigureContext configureContext, StoreContext storeContext) {
        this.configureContext = configureContext;
        this.storeContext = storeContext;
        nativeInit(this.configureContext, this.storeContext);
    }
    protected static void addExternRef(String key, Object val) {
        externRefMap.put(key, val);
    }

    protected static Object getExternRef(String key) {
        return externRefMap.get(key);
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

    public void runWasmFromBuffer(byte[] buffer, String funcName, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
        WasmEdgeValue[] paramsArray = valueListToArray(params);
        int[] paramTypes = getValueTypeArray(params);

        WasmEdgeValue[] returnsArray = valueListToArray(returns);
        int[] returnTypes = getValueTypeArray(returns);

        runWasmFromBuffer(buffer, funcName, paramsArray, paramTypes, returnsArray, returnTypes);
    }

    private native void runWasmFromBuffer(byte[] buffer, String funcName, WasmEdgeValue[] params, int[] paramTypes, WasmEdgeValue[] returns, int[] returnTypes);


    public void runWasmFromASTModule(ASTModuleContext astModuleContext, String funcName, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
        WasmEdgeValue[] paramsArray = valueListToArray(params);
        int[] paramTypes = getValueTypeArray(params);

        WasmEdgeValue[] returnsArray = valueListToArray(returns);
        int[] returnTypes = getValueTypeArray(returns);

        runWasmFromASTModule(astModuleContext, funcName, paramsArray, paramTypes, returnsArray, returnTypes);
    }

    private native void runWasmFromASTModule(ASTModuleContext astModuleContext, String funcName, WasmEdgeValue[] params, int[] paramTypes, WasmEdgeValue[] returns, int[] returnTypes);


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

    public native void registerModuleFromFile(String modName, String filePath);

    public native void registerModuleFromBuffer(String moduleName, byte[] buffer);

    public native void registerModuleFromImport(ImportObjectContext importObjectContext);

    public native void registerModuleFromASTModule(String moduleName, ASTModuleContext astModuleContext);

    public void executeRegistered(String modName, String funcName, List<WasmEdgeValue> params,
                                         List<WasmEdgeValue> returns) {
        WasmEdgeValue[] paramsArray = valueListToArray(params);
        int[] paramTypes = getValueTypeArray(params);

        WasmEdgeValue[] returnsArray = valueListToArray(returns);
        int[] returnTypes = getValueTypeArray(returns);
        executeRegistered(modName, funcName, paramsArray, paramTypes, returnsArray, returnTypes);
    }

    private native void executeRegistered(String modName, String funcName, WasmEdgeValue[] params,
                                         int[] paramTypes, WasmEdgeValue[] returns, int[] returnTypes);

    private native void getFunctionList(List<FunctionTypeContext> functionList);

    public List<FunctionTypeContext> getFunctionList() {
        List<FunctionTypeContext>  funcList = new ArrayList<>();
        getFunctionList(funcList);
        return funcList;
    }

    public native FunctionTypeContext getFunctionType(String funcName);

    public ImportObjectContext getImportModuleContext(HostRegistration reg) {
        return nativeGetImportModuleContext(reg.getVal());
    }

    private native ImportObjectContext nativeGetImportModuleContext(int reg);


    public native StoreContext getStoreContext();
    public native StatisticsContext getStatisticsContext();
    public native FunctionTypeContext getFunctionTypeRegistered(String moduleName,
                                                  String funcName);

    public native void cleanUp();
    private native void delete();
}
