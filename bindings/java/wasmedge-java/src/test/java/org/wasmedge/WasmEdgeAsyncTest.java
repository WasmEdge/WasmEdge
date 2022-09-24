package org.wasmedge;

import java.util.ArrayList;
import java.util.List;

import org.junit.Test;
import org.wasmedge.enums.HostRegistration;

import org.junit.Assert;

public class WasmEdgeAsyncTest extends BaseTest{
    @Test
    public void testAsyncGet() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        WasmEdgeAsync async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        async.wasmEdge_AsyncGet(returns);
        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testAsyncGetLength() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        WasmEdgeAsync async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        int len = async.wasmEdge_AsyncGetReturnsLength();
        Assert.assertEquals(1, len);
        vm.destroy();
    }

    @Test
    public void testAsyncWait() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        WasmEdgeAsync async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        async.wasmEdge_AsyncWait();
        async.wasmEdge_AsyncGet(returns);
        Assert.assertEquals(3, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }

    @Test
    public void testAsyncWaitForInTime() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        WasmEdgeAsync async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        boolean isEnd = async.wasmEdge_AsyncWaitFor(100);
        Assert.assertEquals(true, isEnd);
        vm.destroy();
    }

    @Test
    public void testAsyncWaitForOutOfTime() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(35));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        WasmEdgeAsync async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        boolean isEnd = async.wasmEdge_AsyncWaitFor(100);
        Assert.assertEquals(false, isEnd);
        vm.destroy();
    }

    @Test
    public void testAsyncCancel() throws Exception {
        ConfigureContext configureContext = new ConfigureContext();
        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(configureContext, null);
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(35));

        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        WasmEdgeAsync async = vm.asyncRunWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params);
        boolean isEnd = async.wasmEdge_AsyncWaitFor(100);
        if (!isEnd){
            async.wasmEdge_AsyncCancel();
        }
        Assert.assertEquals(0, ((WasmEdgeI32Value) returns.get(0)).getValue());
        vm.destroy();
    }
    
}
