package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.ValueType;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class WasmEdgeVmTest extends BaseTest {

    @Test
    public void testRun() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        vm.runWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params, returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testRunStepByStep() {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), null);
        vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
        vm.validate();
        vm.instantiate();
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());
        vm.execute("fib", params, returns);

        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test(expected = Exception.class)
    public void testInvalidPath() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile("/root/invalid_path.wasm");
    }

    @Test(expected = Exception.class)
    public void testInvalidFuncName() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
        vm.validate();
        vm.instantiate();
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());
        vm.execute(UUID.randomUUID().toString(), params, returns);
    }

    @Test(expected = Exception.class)
    public void testInvalidFlow() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
        vm.instantiate();
        vm.validate();
    }

    @Test
    public void testRegisterModuleFromFile() {
        WasmEdgeVm vm = new WasmEdgeVm(null, null);
        String modName = "module";
        vm.registerModuleFromFile(modName, getResourcePath(FIB_WASM_PATH));
        vm.destroy();
    }

    @Test
    public void testGetFunctionList() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
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
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
        vm.validate();
        vm.instantiate();
        FunctionTypeContext function = vm.getFunctionType(FUNC_NAME);

        Assert.assertEquals(function.getParameters().size(), 1);
        Assert.assertEquals(function.getParameters().get(0), ValueType.i32);

        Assert.assertEquals(function.getReturns().size(), 1);
        Assert.assertEquals(function.getReturns().get(0), ValueType.i32);
    }

    @Test
    public void testRegisterModuleFromImport() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), null);
        ModuleInstanceContext moduleInstanceContext = new ModuleInstanceContext("extern");
        vm.registerModuleFromImport(moduleInstanceContext);
    }

    @Test
    public void testRegisterModuleFromBuffer() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        vm.registerModuleFromBuffer("module", loadFile(getResourcePath(FIB_WASM_PATH)));
        vm.destroy();
    }

    @Test
    public void testExecuteRegisterModule() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        String modName = "module";
        vm.registerModuleFromBuffer(modName, loadFile(getResourcePath(FIB_WASM_PATH)));

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        vm.executeRegistered(modName, FUNC_NAME, params, returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testRegisterModuleFromAstModule() {
        LoaderContext loaderContext = new LoaderContext(new ConfigureContext());
        AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));

        WasmEdgeVm vm = new WasmEdgeVm(null, null);
        vm.registerModuleFromAstModule("module", mod);
        mod.delete();
        loaderContext.delete();
        vm.destroy();
    }

    @Test
    public void testRunWasmFromBuffer() {
        byte[] data = loadFile(getResourcePath(FIB_WASM_PATH));
        WasmEdgeVm vm = new WasmEdgeVm(null, null);

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());
        vm.runWasmFromBuffer(data, FUNC_NAME, params, returns);

        I32Value value = (I32Value) returns.get(0);
        Assert.assertEquals(3, value.getValue());
    }

    @Test
    public void testRunWasmFromASTModule() {
        LoaderContext loaderContext = new LoaderContext(new ConfigureContext());
        AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));

        WasmEdgeVm vm = new WasmEdgeVm(null, null);

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        vm.runWasmFromAstModule(mod, FUNC_NAME, params, returns);
        I32Value value = (I32Value) returns.get(0);
        Assert.assertEquals(3, value.getValue());
    }
    
    @Test
    public void testAsyncRun() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        async.get(returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testAsyncExecute() {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), null);
        vm.loadWasmFromFile(getResourcePath(FIB_WASM_PATH));
        vm.validate();
        vm.instantiate();
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());
        Async async = vm.asyncExecute("fib", params);
        async.get(returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testAsyncExecuteRegisterModule() {
        WasmEdgeVm vm = new WasmEdgeVm(new ConfigureContext(), new StoreContext());
        String modName = "module";
        vm.registerModuleFromBuffer(modName, loadFile(getResourcePath(FIB_WASM_PATH)));

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncExecuteRegistered(modName, FUNC_NAME, params);
        async.get(returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testAsyncRunWasmFromBuffer() {
        byte[] data = loadFile(getResourcePath(FIB_WASM_PATH));
        WasmEdgeVm vm = new WasmEdgeVm(null, null);

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());
        Async async = vm.asyncRunWasmFromBuffer(data, FUNC_NAME, params);
        async.get(returns);
        I32Value value = (I32Value) returns.get(0);
        Assert.assertEquals(3, value.getValue());
    }

    @Test
    public void testAsyncRunWasmFromASTModule() {
        LoaderContext loaderContext = new LoaderContext(new ConfigureContext());
        AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));

        WasmEdgeVm vm = new WasmEdgeVm(null, null);

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromAstModule(mod, FUNC_NAME, params);
        async.get(returns);
        I32Value value = (I32Value) returns.get(0);
        Assert.assertEquals(3, value.getValue());
    }
}
