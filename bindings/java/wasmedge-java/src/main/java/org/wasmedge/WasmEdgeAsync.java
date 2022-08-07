package org.wasmedge;

public class WasmEdgeAsync {
    private long pointer;

    public native void wasmEdge_AsyncWait();

    public native boolean wasmEdge_AsyncWaitFor(
                               long milliseconds);

    public native void wasmEdge_AsyncCancel();

    public native int wasmEdge_AsyncGetReturnsLength();

    public native Result wasmEdge_AsyncGet(
             WasmEdgeValue returns, int returnLen);

    public native void wasmEdge_AsyncDelete();

}

