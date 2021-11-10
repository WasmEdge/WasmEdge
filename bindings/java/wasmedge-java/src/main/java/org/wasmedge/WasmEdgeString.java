package org.wasmedge;

public class WasmEdgeString {
    private final String val;


    public WasmEdgeString() {
        val = "";
        createInternal(val);
    }
    public WasmEdgeString(String val) {
        this.val = val;
        createInternal(val);
    }

    private native void createInternal(String str);

    private native String toStringInternal();

    private native boolean equalsInternal(WasmEdgeString otherStr);

    @Override
    public String toString() {
        return toStringInternal();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof WasmEdgeString) {
            return equalsInternal((WasmEdgeString)obj);
        }
        return false;
    }
}
