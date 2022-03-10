package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.RefType;

public class TableTypeContextTest extends BaseTest {

    @Test
    public void testCreation() {
        RefType refType = RefType.EXTERREF;
        WasmEdgeLimit limit = new WasmEdgeLimit(true, 1, 1000);
        TableTypeContext tableTypeContext = new TableTypeContext(refType, limit);

        Assert.assertEquals(tableTypeContext.getRefType(), refType);
        Assert.assertEquals(tableTypeContext.getLimit().isHasMax(), limit.isHasMax());
        Assert.assertEquals(tableTypeContext.getLimit().getMax(), limit.getMax());
        Assert.assertEquals(tableTypeContext.getLimit().getMin(), limit.getMin());
    }
}
