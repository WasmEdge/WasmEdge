package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

import java.util.List;
import java.util.UUID;

public class ModuleInstanceContextTest extends BaseTest {
    String[] args = new String[] {"arg1", "arg2"};
    String[] envs = new String[] {"ENV1=VAL1", "ENV2=VAL2", "ENV3=VAL3"};
    String[] preopens = new String[] {"apiTestData", "Makefile", "CMakefiles", "ssvmAPICoreTests", ".:."};
    String[] FUNC_NAMES = {"func-1", "func-2", "func-3", "func-4",
            "func-add", "func-call-indirect", "func-host-add",
            "func-host-div", "func-host-mul", "func-host-sub", "func-mul-2"};


    private ModuleInstanceContext initModuleInstance() {
        ConfigureContext conf = new ConfigureContext();
        StoreContext storeContext = new StoreContext();

        // host mod
        ModuleInstanceContext hostMod = createExternModule("extern");
        ExecutorContext executorContext = new ExecutorContext(conf, null);
        executorContext.registerImport(storeContext, hostMod);

        // load mod
        AstModuleContext mod = loadMod(conf, TEST_WASM_PATH);
        ValidatorContext validator = new ValidatorContext(conf);
        validator.validate(mod);
        executorContext.register(storeContext, mod, "module");
        ModuleInstanceContext moduleInstanceContext = executorContext.instantiate(storeContext, mod);

        return moduleInstanceContext;
    }

    @Test
    public void testCreate() {
        ModuleInstanceContext moduleInstanceContext = new ModuleInstanceContext("extern");
        Assert.assertNotNull(moduleInstanceContext);
    }

    @Test
    public void testAddHostFunction() {
        HostFunction addHostFunc = new HostFunction() {
            @Override
            public Result apply(MemoryInstanceContext mem, List<Value> params, List<Value> returns) {
                return new Result();
            }
        };
        FunctionTypeContext addType = new FunctionTypeContext(new ValueType[] {ValueType.i32, ValueType.i32}, new ValueType[] {ValueType.i32});

        FunctionInstanceContext add = new FunctionInstanceContext(addType, addHostFunc, null, 0);

        ModuleInstanceContext moduleInstanceContext = new ModuleInstanceContext("extern");
        moduleInstanceContext.addFunction("add", add);
    }

    @Test
    public void testAddHostTable() {
        Limit limit = new Limit(true, 10, 20);
        TableTypeContext tableType = new TableTypeContext(RefType.FUNCREF, limit);
        TableInstanceContext tabIns = new TableInstanceContext(tableType);
        ModuleInstanceContext impCxt = new ModuleInstanceContext("extern");
        impCxt.addTable("table", tabIns);
    }

    @Test
    public void testAddHostMemory() {
        Limit limit = new Limit(true, 1, 2);
        MemoryTypeContext memType = new MemoryTypeContext(limit);
        MemoryInstanceContext memIns = new MemoryInstanceContext(memType);
        ModuleInstanceContext impCxt = new ModuleInstanceContext("extern");
        impCxt.addMemory("memory", memIns);
    }

    @Test
    public void testAddHostGlobal() {
        GlobalTypeContext glbType = new GlobalTypeContext(ValueType.i32, Mutability.CONST);
        GlobalInstanceContext glbIns = new GlobalInstanceContext(glbType, new I32Value(666));
        ModuleInstanceContext impCxt = new ModuleInstanceContext("extern");
        impCxt.addGlobal("global_i32", glbIns);
    }

    @Test
    public void testCreateWASI() {
        ModuleInstanceContext moduleInstanceContext = ModuleInstanceContext.createWasi(args, envs, preopens);
        int code = moduleInstanceContext.getWasiExitCode();
        Assert.assertEquals(0, code);
    }

    @Test
    public void testInitWasiInVM() {
        ConfigureContext config = new ConfigureContext();
        config.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(config, null);
        ModuleInstanceContext moduleInstanceContext = vm.getImportModuleContext(HostRegistration.WasmEdge_HostRegistration_Wasi);
        moduleInstanceContext.initWasi(args, envs, preopens);
    }

    @Test
    public void testCreateWasmEdgeProcess() {

        ModuleInstanceContext moduleInstanceContext = ModuleInstanceContext.createWasmEdgeProcess(args, false);
        Assert.assertNotNull(moduleInstanceContext);
    }

    @Test
    public void testInitWasmEdgeProcessInVM() {
        ConfigureContext config = new ConfigureContext();
        config.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_WasmEdge_Process);
        WasmEdgeVm vm = new WasmEdgeVm(config, null);
        ModuleInstanceContext moduleInstanceContext = vm.getImportModuleContext(HostRegistration.WasmEdge_HostRegistration_WasmEdge_Process);
        moduleInstanceContext.initWasmEdgeProcess(args, false);
    }

    @Test
    public void testFindFunction() {
        ModuleInstanceContext moduleInstanceContext = initModuleInstance();
        List<String> funcNames = moduleInstanceContext.listFunction();
        Assert.assertEquals(11, funcNames.size());

        for (int i = 0; i < FUNC_NAMES.length; i++) {
            Assert.assertEquals(FUNC_NAMES[i], funcNames.get(i));
        }

        // find by name
        Assert.assertNotNull(moduleInstanceContext.findFunction(FUNC_NAMES[0]));
        Assert.assertNull(moduleInstanceContext.findFunction(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindTable() {
        ModuleInstanceContext moduleInstanceContext = initModuleInstance();

        //list table exports

        List<String> tabList = moduleInstanceContext.listTable();
        Assert.assertEquals(2, tabList.size());

        Assert.assertEquals("tab-ext", tabList.get(0));
        Assert.assertEquals("tab-func", tabList.get(1));

        // find table
        Assert.assertNotNull(moduleInstanceContext.findTable("tab-ext"));
        Assert.assertNull(moduleInstanceContext.findTable(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindMemory() {
        ModuleInstanceContext moduleInstanceContext = initModuleInstance();
        // list memory exports
        List<String> memList = moduleInstanceContext.listMemory();
        Assert.assertEquals(1, memList.size());
        Assert.assertEquals("mem", memList.get(0));

        // find memory
        Assert.assertNotNull(moduleInstanceContext.findMemory(memList.get(0)));
        Assert.assertNull(moduleInstanceContext.findMemory(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindGlobal() {
        ModuleInstanceContext moduleInstanceContext = initModuleInstance();
        // list global exports
        List<String> globalList = moduleInstanceContext.listGlobal();
        Assert.assertEquals(2, globalList.size());
        Assert.assertEquals("glob-const-f32", globalList.get(0));
        Assert.assertEquals("glob-mut-i32", globalList.get(1));

        // find global
        Assert.assertNotNull(moduleInstanceContext.findGlobal(globalList.get(0)));
        Assert.assertNull(moduleInstanceContext.findGlobal(UUID.randomUUID().toString()));
    }
}
