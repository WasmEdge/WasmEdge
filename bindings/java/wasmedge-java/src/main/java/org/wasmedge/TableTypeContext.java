package org.wasmedge;

import org.wasmedge.enums.RefType;

/**
 * Table type.
 */
public class TableTypeContext extends NativeResource {

    public TableTypeContext(RefType refType, Limit limit) {
        nativeInit(refType.getVal(), limit);
    }

    private TableTypeContext(long pointer) {
        super(pointer);
    }

    private native void nativeInit(int refType, Limit limit);

    public native Limit getLimit();

    public RefType getRefType() {
        return RefType.getType(nativeGetRefType());
    }

    private native int nativeGetRefType();

    public native void close();

}
