import org.junit.Test;
import org.wasmedge.MemoryTypeContext;
import org.wasmedge.WasmEdgeLimit;

public class MemoryTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        MemoryTypeContext memoryTypeContext = new MemoryTypeContext(new WasmEdgeLimit());
    }
}
