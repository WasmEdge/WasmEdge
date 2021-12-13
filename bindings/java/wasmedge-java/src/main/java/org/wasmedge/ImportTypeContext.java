package org.wasmedge;

import org.wasmedge.enums.ExternalType;

public class ImportTypeContext {
    private ASTModuleContext astCtx;

    public native String getModuleName();
    public native String getExternalName();

    public native FunctionTypeContext getFunctionType();

    public native TableTypeContext getTableType();

    public native MemoryTypeContext getMemoryType();

    public native GlobalTypeContext getGlobalType();

    public native ExternalType getExternalType();

}
