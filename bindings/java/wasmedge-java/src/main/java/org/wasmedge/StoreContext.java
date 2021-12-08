package org.wasmedge;

public class StoreContext {
    private long pointer = 0;

    public StoreContext() {
        nativeInit();
    }

    public void destroy() {
        delete();
        pointer = 0;
    }

    private native void nativeInit();
    private native void delete();

    private native String listFunction();

    public native FunctionInstanceContext findFunction(String funcName);

    public native FunctionInstanceContext findFunctionRegistered(String funcName);
}
