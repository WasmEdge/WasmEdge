import org.junit.Test;
import org.wasmedge.ASTModuleContext;
import org.wasmedge.ConfigureContext;
import org.wasmedge.LoaderContext;

public class LoaderContextTest extends BaseTest {
    @Test
    public void testCreation() {
        ConfigureContext configureContext = new ConfigureContext();
        LoaderContext loaderContext = new LoaderContext(configureContext);
        ASTModuleContext moduleContext = loaderContext.parseFromFile(WASM_PATH);

    }
}
