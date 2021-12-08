package org.wasmedge;

public class TableInstanceContext {
    private long pointer;
    public TableInstanceContext(TableTypeContext tableTypeContext) {

    }

    public native void delete();

    public native TableTypeContext getTableType();

    public native void setData(WasmEdgeValue value, int index);

    public native int getSize();

    public native int grow(int size);

}
