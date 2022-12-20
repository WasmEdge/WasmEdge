package org.wasmedge;

import org.junit.Test;

public class ValidatorContextTest extends BaseTest {

    @Test
    public void testCreation() {
        ConfigureContext configureContext = new ConfigureContext();

        LoaderContext loaderContext = new LoaderContext(configureContext);

        AstModuleContext astModuleContext = loaderContext.parseFromFile(getResourcePath(TEST_WASM_PATH));

        ValidatorContext validatorContext = new ValidatorContext(null);
        validatorContext.validate(astModuleContext);

        astModuleContext.delete();
        configureContext.destroy();
        validatorContext.delete();
    }
}
