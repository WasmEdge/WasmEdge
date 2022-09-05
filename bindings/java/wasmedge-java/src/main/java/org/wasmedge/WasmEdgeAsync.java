package org.wasmedge;

public class WasmEdgeAsync {
    private long pointer;

    public native void wasmEdge_AsyncWait();

    public native boolean wasmEdge_AsyncWaitFor(
                               long milliseconds);

    public native void wasmEdge_AsyncCancel();

    public native int wasmEdge_AsyncGetReturnsLength();

    // turn returns to an array
    private native void wasmEdge_AsyncGet(
             WasmEdgeValue[] returns, int[] returnTypes);

    public void wasmEdge_AsyncGet(
             List<WasmEdgeValue> returns);

    public native void wasmEdge_AsyncDelete();

}

