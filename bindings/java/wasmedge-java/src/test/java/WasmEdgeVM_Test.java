import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.WasmEdgeVM;

public class WasmEdgeVM_Test {
    @Test
    public void testVersion() {
        WasmEdgeVM vm = new WasmEdgeVM();
        Assert.assertNotNull(vm.getVersion());
        Assert.assertTrue(vm.getMajorVersion() >= 0);
        Assert.assertTrue(vm.getMinorVersion() >= 0);
        Assert.assertTrue(vm.getPatchVersion() >= 0);
    }
}
