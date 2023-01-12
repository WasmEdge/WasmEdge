package org.wasmedge;

/**
 * function instance context.
 */
public class FunctionInstanceContext {
    private long pointer;

    private FunctionInstanceContext(long pointer) {
        this.pointer = pointer;
    }

    public FunctionInstanceContext(FunctionTypeContext type,
                                   HostFunction hostFunction, Object data,
                                   long cost) {
        String funcKey = WasmEdgeVm.addHostFunc(hostFunction);
        nativeCreateFunction(type, funcKey, data, cost);
    }


    public FunctionInstanceContext(FunctionTypeContext type,
                                   WrapFunction wrapFunction, Object binding,
                                   Object data, long cost) {
        nativeCreateBinding(type, wrapFunction, binding, data, cost);
    }

    private native void nativeCreateFunction(FunctionTypeContext typeContext,
                                             String funcKey, Object data, long cost);

    private native void nativeCreateBinding(FunctionTypeContext typeContext,
                                            WrapFunction wrapFunction,
                                            Object binding,
                                            Object data,
                                            long cost);

    public native void delete();


}
