package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

public class GlobalInstanceContextTest extends BaseTest {
    private GlobalTypeContext typeCxt;
    private GlobalInstanceContext instCxt;

    @Test
    public void testCreation() {
        typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
        instCxt = new GlobalInstanceContext(typeCxt, new I64Value(66666666666L));

        typeCxt.delete();
        instCxt.delete();
    }

    @Test
    public void testGetValType() {
        typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
        instCxt = new GlobalInstanceContext(typeCxt, new I64Value(66666666666L));

        GlobalTypeContext globalTypeContext = instCxt.getGlobalType();
        Assert.assertEquals(globalTypeContext.getValueType(), typeCxt.getValueType());
        Assert.assertEquals(globalTypeContext.getMutability(), typeCxt.getMutability());


        typeCxt.delete();
        instCxt.delete();
    }

    @Test
    public void testGetValue() {
        typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
        instCxt = new GlobalInstanceContext(typeCxt, new I64Value(66666666666L));

        Value value = instCxt.getValue();
        Assert.assertTrue(value instanceof I64Value);
        Assert.assertEquals(((I64Value) value).getValue(), 66666666666L);

        instCxt.setValue(new I64Value(111L));

        value = instCxt.getValue();
        Assert.assertTrue(value instanceof I64Value);
        Assert.assertEquals(((I64Value) value).getValue(), 111L);

        typeCxt.delete();
        instCxt.delete();
    }
}