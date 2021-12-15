import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.MemoryInstanceContext;
import org.wasmedge.MemoryTypeContext;
import org.wasmedge.WasmEdgeLimit;

public class MemoryInstanceTest extends BaseTest {
    @Test
    public void test() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new WasmEdgeLimit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        memCxt.delete();
        memType.delete();
    }

    @Test
    public void testSetDataAndGetData() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new WasmEdgeLimit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        byte[] data = "testdata".getBytes();

        memCxt.setData(data, 100, 10);;

        byte[] buf = new byte[10];
        memCxt.getData(buf, 100 , 10);

        memCxt.delete();
        memType.delete();
    }
    @Test
    public void testGetPointer() {
        Assert.fail("not implemented");
    }


}
