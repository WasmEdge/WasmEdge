package org.wasmedge;

import java.util.List;

public class WasmEdgeAsync {
    private long pointer;

    private WasmEdgeAsync(long pointer) {
        this.pointer = pointer;
    }

    public native void wasmEdge_AsyncWait();

    public native boolean wasmEdge_AsyncWaitFor(
                               long milliseconds);

    public native void wasmEdge_AsyncCancel();

    public native int wasmEdge_AsyncGetReturnsLength();

    // turn returns to an array
    private native void wasmEdge_AsyncGet(
            Value[] returns, int[] returnTypes);

    public void wasmEdge_AsyncGet(
            List<Value> returns) {

            Value[] valuesArray = new Value[returns.size()];
            returns.toArray(valuesArray);
            int[] types = new int[returns.size()];

            for (int i = 0; i < returns.size(); i++) {
                types[i] = returns.get(i).getType().ordinal();
            }
            wasmEdge_AsyncGet(valuesArray, types);
    }

    public native void wasmEdge_AsyncDelete();

}

