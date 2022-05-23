package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;

public class TableInstanceTest extends BaseTest {
    @Test
    public void test() {
        TableTypeContext tab = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabInstance = new TableInstanceContext(tab);

        tab.delete();
        tabInstance.delete();
    }

    @Test
    public void testGetTableType() {
        TableTypeContext tab = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabInstance = new TableInstanceContext(tab);
        Assert.assertEquals(tabInstance.getTableType().getRefType(), RefType.EXTERREF);

        tab.delete();
        tabInstance.delete();
    }

    @Test
    public void testSetAndGetFuncRefData() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.FUNCREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        int idx = 1;
        WasmEdgeValue val = new WasmEdgeFuncRef(1);

        tabIns.setData(val, 5);

        WasmEdgeFuncRef returnRef = (WasmEdgeFuncRef) tabIns.getData(ValueType.ExternRef, 5);
        Assert.assertEquals(idx, returnRef.getIndex());
    }

    @Test(expected = Exception.class)
    public void testSetDataInvalid() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.FUNCREF,
                new WasmEdgeLimit(true, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        WasmEdgeValue val = new WasmEdgeFuncRef(1);
        tabIns.setData(val, 12);
    }

    @Test
    public void testGetSizeAndGrow() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        Assert.assertEquals(tabIns.getSize(), 10);
        tabIns.grow(8);
        Assert.assertEquals(tabIns.getSize(), 18);
    }
}
