package org.wasmedge;

public class FunctionInstanceContext {
    private long pointer;

    public FunctionInstanceContext(FunctionTypeContext type,
                                   HostFunction hostFunction, Object data,
                                   long cost) {

    }

    public FunctionInstanceContext(FunctionTypeContext type,
                               WrapFunction wrapFunction, Object binding,
                               Object data, long cost) {

    }

    public native FunctionTypeContext getFunctionType();

    public native void delete();


}
