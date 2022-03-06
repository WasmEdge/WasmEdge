package org.wasmedge;

import org.wasmedge.enums.RefType;

public class TableTypeContext {

    private long pointer;

    public TableTypeContext(RefType refType, WasmEdgeLimit limit) {
        nativeInit(refType.getVal(), limit);
    }

    private TableTypeContext(long pointer) {
        this.pointer = pointer;
    }

    private native void nativeInit(int refType, WasmEdgeLimit limit);

    public native WasmEdgeLimit getLimit();

    public RefType getRefType() {
        return RefType.getType(nativeGetRefType());
    }
    private native int nativeGetRefType();
    public native void delete();



}
