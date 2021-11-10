package org.wasmedge;

import java.util.List;

public class WasmEdgeVM {

    private native void runWasmFromFile(String file,
                                                      String funcName,
                                                      WasmEdgeValue[] params,
                                                      int paramSize,
                                                      int[] paramTypes,
                                                      WasmEdgeValue[] returns,
                                                      int returnSize,
                                                      int[] returnTypes
                                                  );

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
}
