package org.wasmedge;

import org.wasmedge.enums.ExternalType;

public class ImportTypeContext {
    private ASTModuleContext astCtx;

    private long pointer;

    private ImportTypeContext(long pointer, ASTModuleContext astCtx) {
        this.pointer = pointer;
        this.astCtx = astCtx;
    }

    public native String getModuleName();
    public native String getExternalName();

    public native ExternalType getExternalType();

    public FunctionTypeContext getFunctionType() {
        return nativeGetFunctionType(astCtx);
    }
    private native FunctionTypeContext nativeGetFunctionType(ASTModuleContext astCtx);

    public TableTypeContext getTableType() {
        return nativeGetTableType(astCtx);
    }

    private native TableTypeContext nativeGetTableType(ASTModuleContext astCtx);

    public MemoryTypeContext getMemoryType() {
        return nativeGetMemoryType(astCtx);
    }

    private native MemoryTypeContext nativeGetMemoryType(ASTModuleContext astCtx);

    public GlobalTypeContext getGlobalType() {
        return nativeGetGlobalType(astCtx);
    }

    private native GlobalTypeContext nativeGetGlobalType(ASTModuleContext astCtx);

}
