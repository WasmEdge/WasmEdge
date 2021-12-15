import org.junit.Before;
import org.wasmedge.ASTModuleContext;
import org.wasmedge.ConfigureContext;
import org.wasmedge.LoaderContext;
import org.wasmedge.ValidatorContext;
import org.wasmedge.WasmEdge;

public class BaseTest {
    protected static final String WASM_PATH = "/root/fibonacci.wasm";
    protected static final String TEST_WASM_PATH = "apiTestData/test.wasm";
    protected static final String IMPORT_WASM_PATH = "apiTestData/import.wasm";
    protected static final String FUNC_NAME = "fib";
    byte[] WASM_MAGIC = {0x00, 0x61, 0x73, 0x60};

    @Before
    public void setUp() {
        WasmEdge.init();
    }

    public static ASTModuleContext loadMode(ConfigureContext configureContext) {
        LoaderContext loaderContext = new LoaderContext(configureContext);
        ASTModuleContext astModuleContext = loaderContext.parseFromFile(TEST_WASM_PATH);
        loaderContext.delete();
        return astModuleContext;
    }

}
