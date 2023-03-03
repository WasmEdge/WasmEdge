package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

public class MemoryInstanceTest extends BaseTest {
    @Test
    public void test() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new Limit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        memCxt.delete();
        memType.delete();
    }

    @Test
    public void testSetDataAndGetData() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new Limit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        byte[] data = {1, 2, 3, 4, 5};

        memCxt.setData(data, 100, data.length);

        memCxt.delete();
        memType.delete();
    }

    @Test
    public void testGetSizeAndGrow() {
        MemoryTypeContext memType =
                new MemoryTypeContext(new Limit(false, 1, 1));
        MemoryInstanceContext memCxt = new MemoryInstanceContext(memType);
        Assert.assertEquals(memCxt.getPageSize(), 1);

        memCxt.growPage(1);
        Assert.assertEquals(memCxt.getPageSize(), 2);

        byte[] data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        memCxt.setData(data, 70000, 10);

        byte[] buf = memCxt.getData(70000, 10);

        Assert.assertArrayEquals(data, buf);

        memCxt.delete();
        memType.delete();
    }
}
