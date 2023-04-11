package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * Table instance.
 */
public class TableInstanceContext extends NativeResource {


    private TableTypeContext tableTypeContext;

    private TableInstanceContext(long pointer) {
        super(pointer);
    }

    public TableInstanceContext(TableTypeContext tableTypeContext) {
        this.tableTypeContext = tableTypeContext;
        nativeInit(tableTypeContext);
    }

    private native void nativeInit(TableTypeContext tableTypeContext);

    public native void close();

    public TableTypeContext getTableType() {
        return this.tableTypeContext;
    }

    public native void setData(Value value, int index);

    public native Value getData(ValueType valueType, int offSet);

    public native int getSize();

    public native void grow(int size);

}
