import org.junit.Test;
import org.wasmedge.FunctionTypeContext;
import org.wasmedge.WasmEdgeValue;

import java.util.ArrayList;
import java.util.List;

public class FunctionTypeContextTest extends BaseTest {

    @Test
    public void test() {
        List<WasmEdgeValue> params = new ArrayList<>();
        List<WasmEdgeValue> returns = new ArrayList<>();

        FunctionTypeContext functionTypeContext = new FunctionTypeContext(params, returns);
        functionTypeContext.delete();
    }
}
