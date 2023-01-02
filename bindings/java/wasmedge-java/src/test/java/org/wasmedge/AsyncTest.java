package org.wasmedge;

import java.util.ArrayList;
import java.util.List;

import org.junit.Test;
import org.wasmedge.enums.HostRegistration;

import org.junit.Assert;

public class AsyncTest extends BaseTest{
    @Test
    public void testAsyncGet() throws Exception {
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
        async.delete();
        vm.destroy();
    }

    @Test
    public void testAsyncGetLength() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        int len = async.getReturnsLength();
        Assert.assertEquals(1, len);
        vm.destroy();
    }

    @Test
    public void testAsyncWait() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        async.asyncWait();
        async.get(returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testAsyncWaitForInTime() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        boolean isEnd = async.waitFor(100);
        Assert.assertEquals(true, isEnd);
        async.delete();
        vm.destroy();
    }

    @Test
    public void testAsyncWaitForOutOfTime() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(35));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        boolean isEnd = async.waitFor(100);
        Assert.assertEquals(false, isEnd);
        if (!isEnd){
            async.cancel();
        }
        vm.destroy();
    }

    @Test
    public void testAsyncCancel() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, null);
        List<Value> params = new ArrayList<>();
        params.add(new I32Value(35));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        Async async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        boolean isEnd = async.waitFor(100);
        if (!isEnd){
            async.cancel();
        }
        Assert.assertEquals(0, ((I32Value) returns.get(0)).getValue());
        vm.destroy();
    }
    
}
