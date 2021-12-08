package org.wasmedge;

public class CompilerContext {
    private long pointer;

    public CompilerContext() {

    }
    private native void nativeInit();

    public native void compile(String inputPath, String outputPath);

    public native void delete();
}
