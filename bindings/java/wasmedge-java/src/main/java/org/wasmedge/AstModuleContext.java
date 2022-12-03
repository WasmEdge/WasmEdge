package org.wasmedge;

import java.util.List;

/**
 * Context for AST Module, including imports and exports.
 */
public class AstModuleContext {
    private long pointer;

    public AstModuleContext() {
    }

    private AstModuleContext(long pointer) {
        this.pointer = pointer;
    }

    public native List<ImportTypeContext> listImports();

    public native List<ExportTypeContext> listExports();

    public native void delete();
}
