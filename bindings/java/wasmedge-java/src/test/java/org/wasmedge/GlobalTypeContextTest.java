package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

public class GlobalTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        ValueType valueType = ValueType.f32;
        Mutability mutability = Mutability.VAR;
        GlobalTypeContext globalTypeContext = new GlobalTypeContext(valueType, mutability);

        Assert.assertEquals(globalTypeContext.getValueType(), valueType);
        Assert.assertEquals(globalTypeContext.getMutability(), mutability);
    }
}
