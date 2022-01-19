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
    public void testSetDataAndGetExternRefData() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        Integer num = 1;
        WasmEdgeValue val = new WasmEdgeExternRef(num);

        tabIns.setData(val, 5);

        WasmEdgeExternRef returnRef = (WasmEdgeExternRef) tabIns.getData(ValueType.ExternRef, 5);
        Integer returnVal = (Integer) returnRef.getExternRefVal();
        Assert.assertEquals(num.intValue(), returnVal.intValue());
    }

    @Test
    public void testSetAndGetFuncRefData() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.FUNCREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        int idx = 1;
        WasmEdgeValue val = new WasmEdgeFunctionRef(1);

        tabIns.setData(val, 5);

        WasmEdgeFunctionRef returnRef = (WasmEdgeFunctionRef) tabIns.getData(ValueType.ExternRef, 5);
        Assert.assertEquals(idx, returnRef.getIndex());
    }

    @Test(expected = RuntimeException.class)
    public void testSetDataInvalid() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(true, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        WasmEdgeValue val = new WasmEdgeExternRef(Integer.valueOf(1));
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
