package org.wasmedge;

/**
 * Compiler context, for compiling wasm native modules.
 */
public class CompilerContext {
    private long pointer;

    public CompilerContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    private native void nativeInit(ConfigureContext configureContext);

    public native void compile(String inputPath, String outputPath);

    public native void delete();
}
