import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.WasmEdgeString;

public class WasmEdgeString_Test {
//    @Test
    public void testCreate() {
        WasmEdgeString emptyStr = new WasmEdgeString();
//        emptyStr.delete();

        String testStr = "test";
        WasmEdgeString str = new WasmEdgeString(testStr);
        Assert.assertEquals(testStr, str.toString());
//        str.delete();
    }

//    @Test
    public void testEquals() {
        String testSTr = "test";
        WasmEdgeString str1 = new WasmEdgeString(testSTr);
        WasmEdgeString str2 = new WasmEdgeString(testSTr);

        Assert.assertTrue(str1.equals(str2));
    }
}
