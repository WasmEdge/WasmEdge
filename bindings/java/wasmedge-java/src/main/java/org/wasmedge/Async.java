package org.wasmedge;

import java.util.List;

/**
 * Async object.
 */
public class Async extends NativeResource {

    private Async(long pointer) {
        super(pointer);
    }

    public native void asyncWait();

    public native boolean waitFor(
                               long milliseconds);

    public native void cancel();

    public native int getReturnsLength();

    // turn returns to an array
    private native void get(
            Value[] returns, int[] returnTypes);

    /**
     * Get return values for an async object.
     */
    public void get(List<Value> returns) {

        Value[] valuesArray = new Value[returns.size()];
        returns.toArray(valuesArray);
        int[] types = new int[returns.size()];

        for (int i = 0; i < returns.size(); i++) {
            types[i] = returns.get(i).getType().ordinal();
        }
        get(valuesArray, types);
    }

    public native void close();
}
