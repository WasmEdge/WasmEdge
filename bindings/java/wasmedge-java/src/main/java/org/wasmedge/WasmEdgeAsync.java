package org.wasmedge;

public class WasmEdgeAsync {
    private long pointer;

    Async async;

    public native void WasmEdge_AsyncWait();

    public native boolean WasmEdge_AsyncWaitFor(
                               long Milliseconds);

    public native void WasmEdge_AsyncCancel();

    public native int WasmEdge_AsyncGetReturnsLength();

    public native Result WasmEdge_AsyncGet(
             WasmEdgeValue Returns, int ReturnLen);

    public native void WasmEdge_AsyncDelete();

}

