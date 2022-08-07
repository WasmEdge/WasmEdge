package org.wasmedge;

public class WasmEdgeAsync<T> {
    private long pointer;

    //
    @SafeVarargs
    public WasmEdgeAsync(T... args){ nativeInit(args);}

    @SafeVarargs
    private native void nativeInit(T... args);

    public native void wasmEdge_AsyncWait();

    public native boolean wasmEdge_AsyncWaitFor(
                               long milliseconds);

    public native void wasmEdge_AsyncCancel();

    public native int wasmEdge_AsyncGetReturnsLength();

    public native Result wasmEdge_AsyncGet(
             WasmEdgeValue returns, int returnLen);

    public native void wasmEdge_AsyncDelete();

}

