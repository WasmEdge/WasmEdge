package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.RefType;

public class TableTypeContextTest extends BaseTest {

    @Test
    public void testCreateExtRef() {
        RefType refType = RefType.EXTERREF;
        Limit limit = new Limit(true, 1, 1000);
        TableTypeContext tableTypeContext = new TableTypeContext(refType, limit);

        Assert.assertEquals(tableTypeContext.getRefType(), refType);
        Assert.assertEquals(tableTypeContext.getLimit().isHasMax(), limit.isHasMax());
        Assert.assertEquals(tableTypeContext.getLimit().getMax(), limit.getMax());
        Assert.assertEquals(tableTypeContext.getLimit().getMin(), limit.getMin());
    }

    @Test
    public void testCreateFunRef() {
        RefType refType = RefType.FUNCREF;
        Limit limit = new Limit(true, 1, 1000);
        TableTypeContext tableTypeContext = new TableTypeContext(refType, limit);

        Assert.assertEquals(tableTypeContext.getRefType(), refType);
        Assert.assertEquals(tableTypeContext.getLimit().isHasMax(), limit.isHasMax());
        Assert.assertEquals(tableTypeContext.getLimit().getMax(), limit.getMax());
        Assert.assertEquals(tableTypeContext.getLimit().getMin(), limit.getMin());
    }

}
