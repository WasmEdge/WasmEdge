import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.WasmEdge;

public class WasmEdge_Test {
    @Test
    public void testVersion() {
        WasmEdge wasmEdge = new WasmEdge();
        Assert.assertNotNull(wasmEdge.getVersion());
        Assert.assertTrue(wasmEdge.getMajorVersion() >= 0);
        Assert.assertTrue(wasmEdge.getMinorVersion() >= 0);
        Assert.assertTrue(wasmEdge.getPatchVersion() >= 0);
    }
}
