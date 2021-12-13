package org.wasmedge;

import org.wasmedge.enums.ExternalType;

public class ExportTypeContext {
    private ASTModuleContext astCtx;

    public native String getModuleName();
    public native String getExternalName();

    public native ExternalType getExternalType();

    public native FunctionTypeContext getFunctionType();

    public native TableTypeContext getTableType();

    public native MemoryTypeContext getMemoryType();

    public native GlobalTypeContext getGlobalType();

}
