package org.wasmedge;

import java.io.IOException;
import org.junit.Test;

public class ValidatorContextTest extends BaseTest {

    @Test
    public void testCreation() throws IOException {
        try(ConfigureContext configureContext = new ConfigureContext();
            LoaderContext loaderContext = new LoaderContext(configureContext);
            AstModuleContext astModuleContext = loaderContext.parseFromFile(getResourcePath(TEST_WASM_PATH))) {
            ValidatorContext validatorContext = new ValidatorContext(null);
            validatorContext.validate(astModuleContext);
        }
    }
}
