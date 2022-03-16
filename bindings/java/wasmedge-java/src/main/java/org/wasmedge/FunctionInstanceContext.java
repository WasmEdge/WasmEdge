package org.wasmedge;

public class FunctionInstanceContext {
    private long pointer;

    public FunctionInstanceContext(FunctionTypeContext type,
                                   HostFunction hostFunction, Object data,
                                   long cost) {
        nativeCreateFunction(type, hostFunction, data, cost);
    }

    private native void nativeCreateFunction(FunctionTypeContext typeContext, HostFunction hostFunction, Object data, long cost);

    public FunctionInstanceContext(FunctionTypeContext type,
                               WrapFunction wrapFunction, Object binding,
                               Object data, long cost) {
        nativeCreateBinding(type, wrapFunction, binding, data, cost);
    }

    private native void nativeCreateBinding(FunctionTypeContext typeContext,
                                            WrapFunction wrapFunction,
                                            Object binding,
                                            Object data,
                                            long cost);

    public native void delete();


}
