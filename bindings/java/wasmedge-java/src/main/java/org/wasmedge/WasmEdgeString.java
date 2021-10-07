package org.wasmedge;

public class WasmEdgeString {
    private long pointer;

    public WasmEdgeString() {
//        createInternal("");
    }
    public WasmEdgeString(String str) {
        createInternal(str);
    }

    private native void createInternal(String str);

    public native void delete();

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
