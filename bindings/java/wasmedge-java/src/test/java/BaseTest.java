import org.junit.Before;
import org.junit.BeforeClass;
import org.wasmedge.WasmEdge;
import org.wasmedge.WasmEdgeVM;

public class BaseTest {
    @Before
    public void setUp() {
        WasmEdge.init();
    }
}
