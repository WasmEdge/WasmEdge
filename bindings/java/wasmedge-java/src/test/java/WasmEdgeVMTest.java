import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.wasmedge.VMContext;
import org.wasmedge.WasmEdge;
import org.wasmedge.WasmEdgeI32Value;
import org.wasmedge.WasmEdgeVM;
import org.wasmedge.WasmEdgeValue;

import java.util.ArrayList;
import java.util.List;

public class WasmEdgeVMTest {
    @Before
    public void setUp() {
        WasmEdge.init();
    }

    @Test
    public void testRun() {
        System.out.println("Start testing");
        WasmEdgeVM vm = new WasmEdgeVM(null, null);
        System.out.println("construct params");
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.runWasmFromFile("/root/fibonacci.wasm", "fibnacci", params, returns);
        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
    }

    @Test
    public void testRunStepByStep(){
        System.out.println("Start testing");
        WasmEdgeVM vm = new WasmEdgeVM(null, null);
        System.out.println("Start testing");
        vm.loadWasmFromFile("/root/fibonacci.wasm");
        vm.validate();
        vm.instantiate();
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.execute("fib", params, returns);

        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());

    }
}
