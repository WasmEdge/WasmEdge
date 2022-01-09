package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.ValueType;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class WasmEdgeVMTest extends BaseTest {

    @Test
    public void testRun() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        vm.runWasmFromFile(getResourcePath(WASM_PATH), FUNC_NAME, params, returns);
        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testRunStepByStep(){
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), null);
        vm.loadWasmFromFile(getResourcePath(WASM_PATH));
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
        vm.loadWasmFromFile(getResourcePath(WASM_PATH));
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
        vm.loadWasmFromFile(getResourcePath(WASM_PATH));
        vm.instantiate();
        vm.validate();
    }

    @Test
    public void testRegisterModuleFromFile() {
        WasmEdgeVM vm = new WasmEdgeVM(null, null);
        String modName = "module";
        vm.registerModuleFromFile(modName, getResourcePath(WASM_PATH));
        vm.destroy();
    }


    @Test
    public void testGetFunctionList() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(getResourcePath(WASM_PATH));
        vm.validate();
        vm.instantiate();
        List<FunctionTypeContext> functionList = vm.getFunctionList();
        Assert.assertEquals(functionList.size(), 1);
        Assert.assertEquals(functionList.get(0).getName(), "fib");
        Assert.assertEquals(functionList.get(0).getReturns().size(), 1);
        Assert.assertEquals(functionList.get(0).getReturns().get(0), ValueType.i32);

        Assert.assertEquals(functionList.get(0).getParameters().size(), 1);
        Assert.assertEquals(functionList.get(0).getParameters().get(0), ValueType.i32);


    }

    @Test
    public void getFunctionByName() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(getResourcePath(WASM_PATH));
        vm.validate();
        vm.instantiate();
        FunctionTypeContext function = vm.getFunctionType(FUNC_NAME);

        Assert.assertEquals(function.getParameters().size(), 1);
        Assert.assertEquals(function.getParameters().get(0), ValueType.i32);

        Assert.assertEquals(function.getReturns().size(), 1);
        Assert.assertEquals(function.getReturns().get(0), ValueType.i32);
    }

//    @Test
    public void testRegisterModuleFromImport() {
        Assert.fail("not implemented");
    }


    @Test
    public void testRegisterModuleFromBuffer() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        vm.registerModuleFromBuffer("module", loadFile(getResourcePath(WASM_PATH)));
        vm.destroy();
    }

    @Test
    public void testExecuteRegisterModule() {
        WasmEdgeVM vm = new WasmEdgeVM(new ConfigureContext(), new StoreContext());
        String modName = "module";
        vm.registerModuleFromBuffer(modName, loadFile(getResourcePath(WASM_PATH)));

        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        vm.executeRegistered(modName, FUNC_NAME, params, returns);
        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }



    @Test
    public void testRegisterModuleFromAstModule() {
        LoaderContext loaderContext = new LoaderContext(new ConfigureContext());
        ASTModuleContext mod = loaderContext.parseFromFile(getResourcePath(WASM_PATH));

        WasmEdgeVM vm = new WasmEdgeVM(null, null);
        vm.registerModuleFromASTModule("module", mod);
        mod.delete();
        loaderContext.delete();
        vm.destroy();
    }

    @Test
    public void testRunWasmFromBuffer() {
        byte[] data = loadFile(getResourcePath(WASM_PATH));
        WasmEdgeVM vm = new WasmEdgeVM(null, null);

        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());
        vm.runWasmFromBuffer(data, FUNC_NAME, params, returns);

        WasmEdgeI32Value value = (WasmEdgeI32Value) returns.get(0);
        Assert.assertEquals(3, value.getValue());
    }

    @Test
    public void testRunWasmFromASTModule() {
        LoaderContext loaderContext = new LoaderContext(new ConfigureContext());
        ASTModuleContext mod = loaderContext.parseFromFile(getResourcePath(WASM_PATH));

        WasmEdgeVM vm = new WasmEdgeVM(null, null);

        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        vm.runWasmFromASTModule(mod, FUNC_NAME, params, returns);
        WasmEdgeI32Value value = (WasmEdgeI32Value) returns.get(0);
        Assert.assertEquals(3, value.getValue());
    }

}
