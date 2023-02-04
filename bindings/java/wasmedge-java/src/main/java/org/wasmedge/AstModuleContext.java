package org.wasmedge;

import java.util.List;

/**
 * Context for AST Module, including imports and exports.
 */
public class AstModuleContext extends NativeResource {

    private AstModuleContext() {
        super();
    }

    private AstModuleContext(long pointer) {
        super(pointer);
    }

    public native List<ImportTypeContext> listImports();

    public native List<ExportTypeContext> listExports();

    public native void close();
}
