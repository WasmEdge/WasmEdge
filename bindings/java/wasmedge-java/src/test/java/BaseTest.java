import org.junit.Before;
import org.wasmedge.WasmEdge;

public class BaseTest {
    protected static final String WASM_PATH = "/root/fibonacci.wasm";
    protected static final String FUNC_NAME = "fib";

    @Before
    public void setUp() {
        WasmEdge.init();
    }
}
