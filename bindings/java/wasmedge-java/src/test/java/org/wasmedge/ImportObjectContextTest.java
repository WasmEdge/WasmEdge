package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.WasmEdgeMutability;

import java.util.List;

public class ImportObjectContextTest extends BaseTest {
    String[] args = new String[] {"arg1", "arg2"};
    String[] envs = new String[] {"ENV1=VAL1", "ENV2=VAL2", "ENV3=VAL3"};
    String[] preopens = new String[] {"apiTestData", "Makefile", "CMakefiles", "ssvmAPICoreTests", ".:."};


    @Test
    public void testCreate() {
        ImportObjectContext importObjectContext = new ImportObjectContext("extern");
        Assert.assertNotNull(importObjectContext);
    }

    @Test
    public void testAddHostFunction() {
        HostFunction addHostFunc = new HostFunction() {
            @Override
            public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
                return new Result();
            }
        };
        FunctionTypeContext addType = new FunctionTypeContext(new ValueType[] {ValueType.i32, ValueType.i32}, new ValueType[] {ValueType.i32});

        FunctionInstanceContext add = new FunctionInstanceContext(addType, addHostFunc, null, 0);

        ImportObjectContext importObjectContext = new ImportObjectContext("extern");
        importObjectContext.addFunction("add", add);
    }

    @Test
    public void testAddHostTable() {
        WasmEdgeLimit limit = new WasmEdgeLimit(true, 10, 20);
        TableTypeContext tableType = new TableTypeContext(RefType.FUNCREF, limit);
        TableInstanceContext tabIns = new TableInstanceContext(tableType);
        ImportObjectContext impCxt = new ImportObjectContext("extern");
        impCxt.addTable("table", tabIns);
    }

    @Test
    public void testAddHostMemory() {
        WasmEdgeLimit limit = new WasmEdgeLimit(true, 1, 2);
        MemoryTypeContext memType = new MemoryTypeContext(limit);
        MemoryInstanceContext memIns = new MemoryInstanceContext(memType);
        ImportObjectContext impCxt = new ImportObjectContext("extern");
        impCxt.addMemory("memory", memIns);
    }

    @Test
    public void testAddHostGlobal() {
        GlobalTypeContext glbType = new GlobalTypeContext(ValueType.i32, WasmEdgeMutability.CONST);
        GlobalInstanceContext glbIns = new GlobalInstanceContext(glbType, new WasmEdgeI32Value(666));
        ImportObjectContext impCxt = new ImportObjectContext("extern");
        impCxt.addGlobal("global_i32", glbIns);
    }

    @Test
    public void testCreateWASI() {
        ImportObjectContext importObjectContext = ImportObjectContext.CreateWASI(args, envs, preopens);
        int code = importObjectContext.getWASIExitCode();
        Assert.assertEquals(0, code);
    }

    @Test
    public void testInitWasiInVM() {
        ConfigureContext config = new ConfigureContext();
        config.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVM vm = new WasmEdgeVM(config, null);
        ImportObjectContext importObjectContext = vm.getImportModuleContext(HostRegistration.WasmEdge_HostRegistration_Wasi);
        importObjectContext.initWASI(args, envs, preopens);
    }

    @Test
    public void testCreateWasmEdgeProcess() {

        ImportObjectContext importObjectContext = ImportObjectContext.createWasmEdgeProcess(args, false);
        Assert.assertNotNull(importObjectContext);
    }

    @Test
    public void testInitWasmEdgeProcessInVM() {
        ConfigureContext config = new ConfigureContext();
        config.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_WasmEdge_Process);
        WasmEdgeVM vm = new WasmEdgeVM(config, null);
        ImportObjectContext importObjectContext = vm.getImportModuleContext(HostRegistration.WasmEdge_HostRegistration_WasmEdge_Process);
        importObjectContext.initWasmEdgeProcess(args, false);
    }

}
