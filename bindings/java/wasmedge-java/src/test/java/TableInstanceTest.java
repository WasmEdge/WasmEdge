import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.TableInstanceContext;
import org.wasmedge.TableTypeContext;
import org.wasmedge.WasmEdgeExternalRef;
import org.wasmedge.WasmEdgeFunctionRef;
import org.wasmedge.WasmEdgeLimit;
import org.wasmedge.WasmEdgeValue;
import org.wasmedge.enums.RefType;

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
    public void testSetDataAndGetData() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        WasmEdgeValue val = new WasmEdgeExternalRef(tabCxt);
        WasmEdgeValue tmpVal = new WasmEdgeFunctionRef(2);

        tabIns.setData(val, 5);
        tabIns.setData(tmpVal, 6);

        WasmEdgeValue returnVal = tabIns.getData(5);
        Assert.fail("TBC");
    }

    @Test(expected = RuntimeException.class)
    public void testSetDataInvalid() {
        TableTypeContext tabCxt = new TableTypeContext(RefType.EXTERREF,
                new WasmEdgeLimit(false, 10, 10));
        TableInstanceContext tabIns = new TableInstanceContext(tabCxt);
        WasmEdgeValue val = new WasmEdgeExternalRef(tabCxt);

        tabIns.setData(val, 5);
        tabIns.setData(val, 5);
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
