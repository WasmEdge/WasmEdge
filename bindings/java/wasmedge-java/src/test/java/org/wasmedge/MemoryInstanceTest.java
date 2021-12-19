package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

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

    @Test
    public void testGetSizeAndGrow() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new WasmEdgeLimit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        Assert.assertEquals(memCxt.getPageSize(), 1);

        memCxt.growPage(1);
        Assert.assertEquals(memCxt.getPageSize(), 2);

        byte[] data = "testdata".getBytes();
        memCxt.setData(data, 70000, 10);

        byte[] buf = new byte[10];
        memCxt.getData(buf, 70000, 10);

        Assert.assertEquals(data, buf);

        memCxt.delete();
        memType.delete();
    }
}
