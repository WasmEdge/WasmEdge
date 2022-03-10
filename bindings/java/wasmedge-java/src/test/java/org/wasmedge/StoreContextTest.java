package org.wasmedge;

import org.junit.Assert;
import org.junit.Ignore;
import org.junit.Test;

public class StoreContextTest extends BaseTest {
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
        ASTModuleContext mod = loadMode(conf, TEST_WASM_PATH);
        ValidatorContext validatorContext = new ValidatorContext(conf);
        validatorContext.validate(mod);
        StoreContext storeContext = new StoreContext();
        ExecutorContext exeCxt = new ExecutorContext(conf, new StatisticsContext());
        exeCxt.registerModule(storeContext, mod, "module");
        exeCxt.instantiate(storeContext, mod);
        Assert.assertEquals(storeContext.listFunction().size(), 11);
    }

//    @Test
//    public void testHostModule() {
//        Assert.fail();
//    }

}
