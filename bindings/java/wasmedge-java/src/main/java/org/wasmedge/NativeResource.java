package org.wasmedge;

import java.io.Closeable;
import java.io.IOException;

/**
 * Base class for native resources.
 */
public abstract class NativeResource implements Closeable {
    private long pointer;

    public NativeResource(long pointer) {
        this.pointer = pointer;
    }

    abstract void destroy();

    @Override
    public void close() throws IOException {
        destroy();
    }
}
