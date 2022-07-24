package org.wasmedge;

public class ValidatorContext {
    private long pointer;

    public ValidatorContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    public native void validate(ASTModuleContext astCtx);

    private native void nativeInit(ConfigureContext configureContext);

    public native void delete();
}
