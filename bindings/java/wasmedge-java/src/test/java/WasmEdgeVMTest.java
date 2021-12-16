import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.ConfigureContext;
import org.wasmedge.FunctionTypeContext;
import org.wasmedge.StoreContext;
import org.wasmedge.WasmEdgeI32Value;
import org.wasmedge.WasmEdgeVM;
import org.wasmedge.WasmEdgeValue;
import org.wasmedge.enums.HostRegistration;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class WasmEdgeVMTest extends BaseTest {

    @Test
    public void testRun() {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.runWasmFromFile(WASM_PATH, FUNC_NAME, params, returns);
        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testRunStepByStep(){
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), null);
        vm.loadWasmFromFile(WASM_PATH);
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

    @Test
    public void testRegisterModule() {
        WasmEdgeVM vm = new WasmEdgeVM(null, null);

        String modName = "mode";
        String funcName = FUNC_NAME;

        vm.registerModuleFromFile(modName, funcName);

        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.executeRegistered(modName, funcName, params, returns);

        vm.destroy();
    }

    @Test
    public void testGetFunctionList() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(WASM_PATH);
        vm.validate();
        vm.instantiate();
        List<FunctionTypeContext> functionList = vm.getFunctionList();
        Assert.assertEquals(functionList.size(), 1);

        FunctionTypeContext function = vm.getFunctionType(FUNC_NAME);
    }

    @Test
    public void testRegisterModuleFromImport() {
        Assert.fail("not implemented");
    }


    @Test
    public void testRegisterModuleFromBuffer() {

    }

    @Test
    public void testRegisterModuleFromAstModule() {

    }

    @Test
    public void testRunWasmFromBuffer() {

    }

    @Test
    public void testRunWasmFromASTModule() {
        
    }

}
