package org.wasmedge;

import org.wasmedge.enums.ExternalType;

/**
 * Context for exported type, including functions, memory, table and globals.
 */
public class ExportTypeContext {
    private final AstModuleContext astCtx;

    private final long pointer;

    private ExportTypeContext(long pointer, AstModuleContext astCtx) {
        this.pointer = pointer;
        this.astCtx = astCtx;
    }

    public native String getModuleName();

    public native String getExternalName();

    public ExternalType getExternalType() {
        return ExternalType.getByValue(nativeGetExternalType());
    }

    private native int nativeGetExternalType();


    public FunctionTypeContext getFunctionType() {
        return nativeGetFunctionType(astCtx);
    }

    private native FunctionTypeContext nativeGetFunctionType(AstModuleContext astCtx);

    public TableTypeContext getTableType() {
        return nativeGetTableType(astCtx);
    }

    private native TableTypeContext nativeGetTableType(AstModuleContext astCtx);

    public MemoryTypeContext getMemoryType() {
        return nativeGetMemoryType(astCtx);
    }

    private native MemoryTypeContext nativeGetMemoryType(AstModuleContext astCtx);

    public GlobalTypeContext getGlobalType() {
        return nativeGetGlobalType(astCtx);
    }

    private native GlobalTypeContext nativeGetGlobalType(AstModuleContext astCtx);

}
