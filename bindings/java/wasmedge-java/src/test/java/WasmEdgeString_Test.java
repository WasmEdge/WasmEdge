import org.junit.Test;
import org.wasmedge.WasmEdgeString;

public class WasmEdgeString_Test {
    @Test
    public void testCreate() {
        WasmEdgeString emptyStr = new WasmEdgeString();
        emptyStr.delete();

        String testStr = "test";
        WasmEdgeString str = new WasmEdgeString(testStr);
        str.delete();
    }
}
