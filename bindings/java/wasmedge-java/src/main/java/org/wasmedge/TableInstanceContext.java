package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * Table instance.
 */
public class TableInstanceContext {

    private long pointer;

    private TableTypeContext tableTypeContext;

    private TableInstanceContext(long pointer) {
        this.pointer = pointer;
    }

    public TableInstanceContext(TableTypeContext tableTypeContext) {
        this.tableTypeContext = tableTypeContext;
        nativeInit(tableTypeContext);
    }

    private native void nativeInit(TableTypeContext tableTypeContext);

    public native void delete();

    public TableTypeContext getTableType() {
        return this.tableTypeContext;
    }

    public native void setData(Value value, int index);

    public native Value getData(ValueType valueType, int offSet);

    public native int getSize();

    public native void grow(int size);

}
