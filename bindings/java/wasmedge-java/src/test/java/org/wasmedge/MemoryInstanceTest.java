package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

public class MemoryInstanceTest extends BaseTest {
    @Test
    public void test() {
        try(MemoryTypeContext memType = new MemoryTypeContext(new Limit(false, 1, 1));
            MemoryInstanceContext memCxt = new MemoryInstanceContext(memType)) {
            Assert.assertNotNull(memType);
            Assert.assertNotNull(memCxt);
        }
    }

    @Test
    public void testSetDataAndGetData() {
        try(MemoryTypeContext memType =
                new MemoryTypeContext(new Limit(false, 1, 1));
            MemoryInstanceContext memCxt = new MemoryInstanceContext(memType)) {
            byte[] data = {1, 2, 3, 4, 5};

            memCxt.setData(data, 100, data.length);

        }
    }

    @Test
    public void testGetSizeAndGrow() {
        try(MemoryTypeContext memType =
                new MemoryTypeContext(new Limit(false, 1, 1));
            MemoryInstanceContext memCxt = new MemoryInstanceContext(memType)) {
            Assert.assertEquals(memCxt.getPageSize(), 1);

            memCxt.growPage(1);
            Assert.assertEquals(memCxt.getPageSize(), 2);

            byte[] data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            memCxt.setData(data, 70000, 10);

            byte[] buf = memCxt.getData(70000, 10);

            Assert.assertArrayEquals(data, buf);

        }
    }
}
