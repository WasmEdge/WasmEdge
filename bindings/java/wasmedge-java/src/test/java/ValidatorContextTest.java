import org.junit.Test;
import org.wasmedge.ASTModuleContext;
import org.wasmedge.ConfigureContext;
import org.wasmedge.LoaderContext;
import org.wasmedge.ValidatorContext;

public class ValidatorContextTest extends BaseTest {

    @Test
    public void testCreation() {
        ConfigureContext configureContext = new ConfigureContext();

        LoaderContext loaderContext = new LoaderContext(configureContext);

        ASTModuleContext astModuleContext = loaderContext.parseFromFile(TEST_WASM_PATH);

        ValidatorContext validatorContext = new ValidatorContext(null);
        validatorContext.validate(astModuleContext);

        astModuleContext.delete();
        configureContext.destroy();
        validatorContext.delete();
    }
}
