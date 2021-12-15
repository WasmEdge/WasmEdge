import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.ASTModuleContext;
import org.wasmedge.ConfigureContext;
import org.wasmedge.StoreContext;
import org.wasmedge.ValidatorContext;

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
        Assert.assertEquals(storeContext.listTableRegistered(modName).size(), 0);
        Assert.assertEquals(storeContext.listModule().size(), 0);
    }

    @Test
    public void testHostModule() {
        Assert.fail();
    }

}
