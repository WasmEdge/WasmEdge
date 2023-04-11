package org.wasmedge;

import java.io.Closeable;

/**
 * Base class for native resources.
 */
public abstract class NativeResource implements Closeable {
    private long pointer;

    protected NativeResource() {
        pointer = 0;
    }

    protected NativeResource(long pointer) {
        this.pointer = pointer;
    }

    public abstract void close();
}
