package org.wasmedge;

/**
 * Compiler context, for compiling wasm native modules.
 */
public class CompilerContext extends NativeResource {

    public CompilerContext(ConfigureContext configureContext) {
        super();
        nativeInit(configureContext);
    }

    private native void nativeInit(ConfigureContext configureContext);

    public native void compile(String inputPath, String outputPath);

    public native void close();
}
