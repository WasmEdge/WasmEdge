package org.wasmedge;

/**
 * Validator, for validating AstModule.
 */
public class ValidatorContext extends NativeResource {

    public ValidatorContext(ConfigureContext configureContext) {
        nativeInit(configureContext);
    }

    public native void validate(AstModuleContext astCtx);

    private native void nativeInit(ConfigureContext configureContext);

    public native void close();
}
