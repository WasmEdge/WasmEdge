package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

public class MemoryTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        Limit limit = new Limit(true, 1, 1000);
        MemoryTypeContext memoryTypeContext = new MemoryTypeContext(limit);

        Assert.assertEquals(memoryTypeContext.getLimit().isHasMax(), limit.isHasMax());
        Assert.assertEquals(memoryTypeContext.getLimit().getMin(), limit.getMin());
        Assert.assertEquals(memoryTypeContext.getLimit().getMax(), limit.getMax());
    }
}
