package org.wasmedge;

/**
 * Global instance.
 */
public class GlobalInstanceContext {
    private GlobalTypeContext globalTypeContext;
    private Value value;
    private long pointer;

    private GlobalInstanceContext(long pointer) {
        this.pointer = pointer;
    }

    /**
     * Create a global instance context.
     *
     * @param typeCxt instance type.
     * @param value value.
     */
    public GlobalInstanceContext(GlobalTypeContext typeCxt,
                                 Value value) {
        this.globalTypeContext = typeCxt;
        this.value = value;
        nativeInit(typeCxt, value);
    }

    private native void nativeInit(GlobalTypeContext typeCxt, Value value);

    public GlobalTypeContext getGlobalType() {
        return globalTypeContext;
    }

    private native void nativeSetValue(Value value);

    public Value getValue() {
        return this.value;
    }

    public void setValue(Value value) {
        this.value = value;
        nativeSetValue(value);
    }

    public native void delete();
}
