import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.FunctionTypeContext;
import org.wasmedge.enums.ValueType;

import java.util.ArrayList;
import java.util.List;

public class FunctionTypeContextTest extends BaseTest {

    @Test
    public void test() {
        List<ValueType> expectedParams = new ArrayList<>();
        expectedParams.add(ValueType.i64);
        List<ValueType> expectedReturns = new ArrayList<>();
        expectedReturns.add(ValueType.f32);

        FunctionTypeContext functionTypeContext = new FunctionTypeContext(expectedParams, expectedReturns);

        List<ValueType> actualParams = functionTypeContext.getParameters();
        List<ValueType> actualReturns = functionTypeContext.getReturns();

        Assert.assertEquals(expectedParams.size(), actualParams.size());
        Assert.assertEquals(expectedParams.get(0), actualParams.get(0));

        Assert.assertEquals(expectedReturns.size(), actualReturns.size());
        Assert.assertEquals(expectedReturns.get(0), actualReturns.get(0));

        functionTypeContext.delete();
    }

}
