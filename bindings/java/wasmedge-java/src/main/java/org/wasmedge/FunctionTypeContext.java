package org.wasmedge;

import java.util.List;

public class FunctionTypeContext {
    private long pointer;

    public FunctionTypeContext(List<WasmEdgeValue> params,
                               List<WasmEdgeValue> returns) {
        nativeInit(params, params.size(), returns, returns.size());
    }

    private native void nativeInit(List<WasmEdgeValue> params, int paramSize,
                                   List<WasmEdgeValue> returns, int returnSize);

    public native List<WasmEdgeValue> getParameters();

    public native List<WasmEdgeValue> getReturns();

    public native void delete();


}
