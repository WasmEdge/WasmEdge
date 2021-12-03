import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.FunctionTypeContext;
import org.wasmedge.WasmEdgeF32Value;
import org.wasmedge.WasmEdgeI32Value;
import org.wasmedge.WasmEdgeValue;

import java.util.ArrayList;
import java.util.List;

public class FunctionTypeContextTest extends BaseTest {

    @Test
    public void test() {
        List<WasmEdgeValue> expectedParams = new ArrayList<>();
        expectedParams.add(new WasmEdgeI32Value(10));
        List<WasmEdgeValue> expectedReturns = new ArrayList<>();
        expectedReturns.add(new WasmEdgeF32Value(10.0f));

        FunctionTypeContext functionTypeContext = new FunctionTypeContext(expectedParams, expectedReturns);

        List<WasmEdgeValue> actualParams = functionTypeContext.getParameters();
        List<WasmEdgeValue> actualReturns = functionTypeContext.getReturns();

        Assert.assertEquals(expectedParams.size(), actualParams.size());
        Assert.assertEquals(expectedParams.get(0), actualParams.get(0));

        Assert.assertEquals(expectedReturns.size(), actualReturns.size());
        Assert.assertEquals(expectedReturns.get(0), actualReturns.get(0));

        functionTypeContext.delete();
    }

}
