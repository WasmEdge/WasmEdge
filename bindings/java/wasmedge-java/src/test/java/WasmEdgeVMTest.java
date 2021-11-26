import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.wasmedge.ConfigureContext;
import org.wasmedge.StoreContext;
import org.wasmedge.WasmEdge;
import org.wasmedge.WasmEdgeI32Value;
import org.wasmedge.WasmEdgeVM;
import org.wasmedge.WasmEdgeValue;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

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
        vm.destroy();
    }

    @Test
    public void testRunStepByStep(){
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile("/root/fibonacci.wasm");
        vm.validate();
        vm.instantiate();
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.execute("fib", params, returns);

        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test(expected = Exception.class)
    public void testInvalidPath () {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile("/root/invalid_path.wasm");
    }

    @Test(expected = Exception.class)
    public void testInvalidFuncName() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile("/root/fibonacci.wasm");
        vm.validate();
        vm.instantiate();
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.execute(UUID.randomUUID().toString(), params, returns);
    }


    @Test(expected = Exception.class)
    public void testInvalidFlow() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile("/root/fibonacci.wasm");
        vm.instantiate();
        vm.validate();
    }
}
