package org.wasmedge;

import org.junit.Assert;
import org.junit.Ignore;
import org.junit.Test;

import java.util.List;
import java.util.UUID;
public class StoreContextTest extends BaseTest {
    String[] FUNC_NAMES = {"func-1", "func-2", "func-3", "func-4",
            "func-add", "func-call-indirect", "func-host-add",
            "func-host-div", "func-host-mul", "func-host-sub", "func-mul-2"};

    @Test
    public void testCreation() {
        String modName = "module";
        StoreContext storeContext = new StoreContext();
        Assert.assertEquals(storeContext.listFunction().size(), 0);
        Assert.assertEquals(storeContext.listTable().size(), 0);
        Assert.assertEquals(storeContext.listMemory().size(), 0);
        Assert.assertEquals(storeContext.listGlobal().size(), 0);
        Assert.assertEquals(storeContext.listFunctionRegistered(modName).size(), 0);
        Assert.assertEquals(storeContext.listMemoryRegistered(modName).size(), 0);
        Assert.assertEquals(storeContext.listTableRegistered(modName).size(), 0);
        Assert.assertEquals(storeContext.listGlobalRegistered(modName).size(), 0);
        Assert.assertEquals(storeContext.listModule().size(), 0);
    }

    @Test
    public void testStore() {
        ConfigureContext conf = new ConfigureContext();
        ASTModuleContext mod = loadMode(conf, FIB_WASM_PATH);
        ValidatorContext validatorContext = new ValidatorContext(conf);
        validatorContext.validate(mod);
        StoreContext storeContext = new StoreContext();
        ExecutorContext exeCxt = new ExecutorContext(conf, new StatisticsContext());
        exeCxt.registerModule(storeContext, mod, "module");
        exeCxt.instantiate(storeContext, mod);
        Assert.assertEquals(storeContext.listFunction().size(), 1);
    }

    private StoreContext initStore() {
        ConfigureContext conf = new ConfigureContext();
        StoreContext store = new StoreContext();
        ASTModuleContext mod = loadMode(conf, TEST_WASM_PATH);
        ImportObjectContext impCxt = createExternModule("extern");

        ExecutorContext executorContext = new ExecutorContext(conf, null);
        executorContext.registerImport(store, impCxt);
        executorContext.registerModule(store, mod, "module");
        executorContext.instantiate(store, mod);
        return store;
    }

    @Test
    public void testFindFunction() {
        StoreContext store = initStore();
        List<String> funcNames = store.listFunction();
        Assert.assertEquals(11, funcNames.size());

        for (int i = 0; i < FUNC_NAMES.length; i++) {
            Assert.assertEquals(FUNC_NAMES[i], funcNames.get(i));
        }

        // find by name
        Assert.assertNotNull(store.findFunction(FUNC_NAMES[0]));
        Assert.assertNull(store.findFunction(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindFunctionRegistered() {
        StoreContext store = initStore();
        // list exports registered
        List<String> funcNames = store.listFunctionRegistered("module");
        Assert.assertEquals(11, funcNames.size());
        Assert.assertEquals(6, store.listFunctionRegistered("extern").size());
        Assert.assertEquals(0, store.listFunctionRegistered("no-such-module").size());
        for (int i = 0; i < FUNC_NAMES.length; i++) {
            Assert.assertEquals(FUNC_NAMES[i], funcNames.get(i));
        }

        // find by name
        Assert.assertNotNull(store.findFunctionRegistered("module", funcNames.get(0)));
        Assert.assertNull(store.findFunctionRegistered("module", UUID.randomUUID().toString()));
    }

    @Test
    public void testFindTable() {
        StoreContext store = initStore();

        //list table exports

        List<String> tabList = store.listTable();
        Assert.assertEquals(2, tabList.size());

        Assert.assertEquals("tab-ext", tabList.get(0));
        Assert.assertEquals("tab-func", tabList.get(1));

        // find table
        Assert.assertNotNull(store.findTable("tab-ext"));
        Assert.assertNull(store.findTable(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindTableRegistered() {
        StoreContext store = initStore();

        // list table exports registered
        List<String> tabList = store.listTableRegistered("module");
        Assert.assertEquals(2, tabList.size());

        Assert.assertEquals("tab-ext", tabList.get(0));
        Assert.assertEquals("tab-func", tabList.get(1));

        // find table registered
        Assert.assertNotNull(store.findTableRegistered("module", tabList.get(0)));
        Assert.assertNull(store.findTableRegistered("module", UUID.randomUUID().toString()));
    }

    @Test
    public void testFindMemory() {
        StoreContext store = initStore();
        // list memory exports
        List<String> memList = store.listMemory();
        Assert.assertEquals(1, memList.size());
        Assert.assertEquals("mem", memList.get(0));

        // find memory
        Assert.assertNotNull(store.findMemory(memList.get(0)));
        Assert.assertNull(store.findMemory(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindMemoryRegistered() {
        StoreContext store = initStore();

        // list memory exports registered
        List<String> memList = store.listMemoryRegistered("module");
        Assert.assertEquals(1, memList.size());
        Assert.assertEquals("mem", memList.get(0));

        // find memory registered
        Assert.assertNotNull(store.findMemoryRegistered("module", memList.get(0)));
        Assert.assertNull(store.findMemoryRegistered("module", UUID.randomUUID().toString()));

    }
    @Test
    public void testFindGlobal() {
        StoreContext store = initStore();
        // list global exports
        List<String> globalList = store.listGlobal();
        Assert.assertEquals(2, globalList.size());
        Assert.assertEquals("glob-const-f32", globalList.get(0));
        Assert.assertEquals("glob-mut-i32", globalList.get(1));

        // find global
        Assert.assertNotNull(store.findGlobal(globalList.get(0)));
        Assert.assertNull(store.findGlobal(UUID.randomUUID().toString()));

    }

    @Test
    public void testFindGlobalRegistered() {
        StoreContext store = initStore();

        //list global exports registered

        List<String> globalList = store.listGlobalRegistered("module");
        Assert.assertEquals(2, globalList.size());

        Assert.assertEquals("glob-const-f32", globalList.get(0));
        Assert.assertEquals("glob-mut-i32", globalList.get(1));


        // find global registered
        Assert.assertNotNull(store.findGlobalRegistered("module", globalList.get(0)));

        Assert.assertNull(store.findGlobalRegistered(UUID.randomUUID().toString(), globalList.get(0)));

    }

    @Test
    public void testFindModule() {
        StoreContext store = initStore();
        // list module
        List<String> modList = store.listModule();
        Assert.assertEquals(2, modList.size());
        Assert.assertEquals("extern", modList.get(0));
        Assert.assertEquals("module", modList.get(1));;

    }

}
