package org.wasmedge;

public class ASTModuleContext {
    private long pointer;

    public ASTModuleContext() {

    }

    public native ImportTypeContext listImports();

    public native ExecutorContext listExports();

    public native void delete();
}
