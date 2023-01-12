package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

import java.util.List;

public class StoreContextTest extends BaseTest {

    @Test
    public void testCreation() {
        StoreContext storeContext = new StoreContext();
        Assert.assertEquals(storeContext.listModule().size(), 0);
    }

    @Test
    public void testStore() {
        ConfigureContext conf = new ConfigureContext();
        AstModuleContext mod = loadMod(conf, FIB_WASM_PATH);
        ValidatorContext validatorContext = new ValidatorContext(conf);
        validatorContext.validate(mod);
        StoreContext storeContext = new StoreContext();
        ExecutorContext exeCxt = new ExecutorContext(conf, new StatisticsContext());
        ModuleInstanceContext moduleInstanceContext = exeCxt.instantiate(storeContext, mod);

        exeCxt.register(storeContext, mod, "module");
        Assert.assertEquals(moduleInstanceContext.listFunction().size(), 1);
    }

    private StoreContext initStore() {
        ConfigureContext conf = new ConfigureContext();
        StoreContext store = new StoreContext();
        // register host mod
        ModuleInstanceContext hostMod = createExternModule("extern");
        ExecutorContext executorContext = new ExecutorContext(conf, null);
        executorContext.registerImport(store, hostMod);

        // instantiate mod
        AstModuleContext mod = loadMod(conf, TEST_WASM_PATH);
        ValidatorContext validatorContext = new ValidatorContext(conf);
        validatorContext.validate(mod);
        executorContext.register(store, mod, "module");
        executorContext.instantiate(store, mod);
        return store;
    }

    @Test
    public void testFindModule() {
        StoreContext store = initStore();
        // list module
        List<String> modList = store.listModule();
        Assert.assertEquals(2, modList.size());
        Assert.assertEquals("extern", modList.get(0));
        Assert.assertEquals("module", modList.get(1));
    }

}
