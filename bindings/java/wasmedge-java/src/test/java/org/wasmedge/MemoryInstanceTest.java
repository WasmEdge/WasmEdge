package org.wasmedge;

import org.junit.Assert;
import org.junit.Ignore;
import org.junit.Test;

public class MemoryInstanceTest extends BaseTest {
    @Test
    @Ignore
    public void test() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new WasmEdgeLimit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        memCxt.delete();
        memType.delete();
    }

    @Test
    @Ignore
    public void testSetDataAndGetData() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new WasmEdgeLimit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        byte[] data = "testdata".getBytes();

        memCxt.setData(data, 100, 10);;

        byte[] buf = memCxt.getData(100 , 10);

        memCxt.delete();
        memType.delete();
    }
    @Test
    @Ignore
    public void testGetPointer() {
        Assert.fail("not implemented");
    }

    @Test
    @Ignore
    public void testGetSizeAndGrow() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new WasmEdgeLimit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        Assert.assertEquals(memCxt.getPageSize(), 1);

        memCxt.growPage(1);
        Assert.assertEquals(memCxt.getPageSize(), 2);

        byte[] data = "testdata".getBytes();
        memCxt.setData(data, 70000, 10);

        byte[] buf = memCxt.getData(70000, 10);

        Assert.assertEquals(data, buf);

        memCxt.delete();
        memType.delete();
    }
}
