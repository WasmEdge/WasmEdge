package org.wasmedge;

public class WasmEdgeAsync {
    Async async;
    static public native void WasmEdge_AsyncWait(WasmEdgeAsync Cxt);

    static public native boolean WasmEdge_AsyncWaitFor(WasmEdgeAsync Cxt,
                               long Milliseconds);

    static public native void WasmEdge_AsyncCancel(WasmEdgeAsync Cxt);

    static public native int WasmEdge_AsyncGetReturnsLength(WasmEdgeAsync Cxt);

    public native Result WasmEdge_AsyncGet(
            WasmEdgeAsync Cxt, WasmEdgeValue Returns, int ReturnLen);

}

