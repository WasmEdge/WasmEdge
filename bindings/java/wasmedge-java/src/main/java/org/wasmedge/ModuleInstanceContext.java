package org.wasmedge;

import java.util.List;

/**
 * Module instance.
 */
public class ModuleInstanceContext {
    private long pointer;

    public ModuleInstanceContext(String moduleName) {
        nativeInit(moduleName);
    }

    private ModuleInstanceContext(long pointer) {
        this.pointer = pointer;
    }

    public static native ModuleInstanceContext createWasmEdgeProcess(String[] allowedCmds,
                                                                     boolean allowAll);

    public static native ModuleInstanceContext createWasi(String[] args, String[] envs,
                                                          String[] preopens);

    private native void nativeInit(String moduleName);

    public native void initWasi(String[] args, String[] envs, String[] preopens);

    public native int getWasiExitCode();

    public native void initWasmEdgeProcess(String[] allowedCmds, boolean allowAll);

    public native void addFunction(String name, FunctionInstanceContext functionInstanceContext);

    public native void addTable(String name, TableInstanceContext tableInstanceContext);

    public native void addMemory(String name, MemoryInstanceContext memoryInstanceContext);

    public native void addGlobal(String name, GlobalInstanceContext globalInstanceContext);

    public native List<String> listFunction();

    public native List<String> listFunctionRegistered(String moduleName);

    public native FunctionInstanceContext findFunction(String funcName);

    public native FunctionInstanceContext findFunctionRegistered(String moduleName,
                                                                 String funcName);

    public native List<String> listTable();

    public native List<String> listTableRegistered(String moduleName);

    public native TableInstanceContext findTable(String tableName);

    public native TableInstanceContext findTableRegistered(String moduleName, String tableName);

    public native List<String> listMemory();

    public native List<String> listMemoryRegistered(String moduleName);

    public native MemoryInstanceContext findMemory(String memoryName);

    public native MemoryInstanceContext findMemoryRegistered(String moduleName, String memoryName);

    public native List<String> listGlobal();

    public native List<String> listGlobalRegistered(String moduleName);

    public native GlobalInstanceContext findGlobal(String name);

    public native GlobalInstanceContext findGlobalRegistered(String moduleName, String globalName);

    public native void delete();
}
