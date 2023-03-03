package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;

public class TableInstanceTest extends BaseTest {
    @Test
    public void test() {
        TableTypeContext tab = new TableTypeContext(RefType.EXTERREF,
                new Limit(false, 10, 10));
        TableInstanceContext tabInstance = new TableInstanceContext(tab);

        tab.delete();
        tabInstance.delete();
    }

    @Test
    public void testGetTableType() {
        TableTypeContext tab = new TableTypeContext(RefType.EXTERREF,
                new Limit(false, 10, 10));
        TableInstanceContext tabInstance = new TableInstanceContext(tab);
        Assert.assertEquals(tabInstance.getTableType().getRefType(), RefType.EXTERREF);

        tab.delete();
        tabInstance.delete();
    }

    @Test
    public void testSetAndGetFuncRefData() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.FUNCREF,
                new Limit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        int idx = 1;
        Value val = new FuncRef(1);

        tabIns.setData(val, 5);

        FuncRef returnRef = (FuncRef) tabIns.getData(ValueType.ExternRef, 5);
        Assert.assertEquals(idx, returnRef.getIndex());
    }

    @Test(expected = Exception.class)
    public void testSetDataInvalid() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.FUNCREF,
                new Limit(true, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        Value val = new FuncRef(1);
        tabIns.setData(val, 12);
    }

    @Test
    public void testGetSizeAndGrow() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.EXTERREF,
                new Limit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        Assert.assertEquals(tabIns.getSize(), 10);
        tabIns.grow(8);
        Assert.assertEquals(tabIns.getSize(), 18);
    }
}
