package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ValueType;

import java.util.ArrayList;
import java.util.List;

public class FunctionTypeContextTest extends BaseTest {

    @Test
    public void test() {
        List<ValueType> expectedParams = new ArrayList<>();
        expectedParams.add(ValueType.i32);
        expectedParams.add(ValueType.i64);
        expectedParams.add(ValueType.ExternRef);
        expectedParams.add(ValueType.v128);
        expectedParams.add(ValueType.f64);
        expectedParams.add(ValueType.f32);

        List<ValueType> expectedReturns = new ArrayList<>();
        expectedReturns.add(ValueType.FuncRef);
        expectedReturns.add(ValueType.ExternRef);
        expectedReturns.add(ValueType.v128);

        FunctionTypeContext functionTypeContext = new FunctionTypeContext(expectedParams, expectedReturns);

        List<ValueType> actualParams = functionTypeContext.getParameters();
        List<ValueType> actualReturns = functionTypeContext.getReturns();

        Assert.assertEquals(expectedParams.size(), actualParams.size());
        for (int i = 0; i < expectedParams.size(); i++) {
            Assert.assertEquals(expectedParams.get(i), actualParams.get(i));
        }

        Assert.assertEquals(expectedReturns.size(), actualReturns.size());

        for (int i = 0; i < expectedReturns.size(); i++) {
            Assert.assertEquals(expectedReturns.get(i), actualReturns.get(i));
        }

        functionTypeContext.delete();
    }

}
