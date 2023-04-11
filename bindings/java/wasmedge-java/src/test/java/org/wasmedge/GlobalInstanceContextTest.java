package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

public class GlobalInstanceContextTest extends BaseTest {

    @Test
    public void testCreation() {
        try(GlobalTypeContext typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
            GlobalInstanceContext instCxt = new GlobalInstanceContext(typeCxt, new I64Value(66666666666L))) {
        }
    }

    @Test
    public void testGetValType() {
        try(GlobalTypeContext typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
            GlobalInstanceContext instCxt = new GlobalInstanceContext(typeCxt, new I64Value(66666666666L))) {

            GlobalTypeContext globalTypeContext = instCxt.getGlobalType();
            Assert.assertEquals(globalTypeContext.getValueType(), typeCxt.getValueType());
            Assert.assertEquals(globalTypeContext.getMutability(), typeCxt.getMutability());
        }
    }

    @Test
    public void testGetValue() {
        try(GlobalTypeContext typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
            GlobalInstanceContext instCxt = new GlobalInstanceContext(typeCxt, new I64Value(66666666666L))) {

            Value value = instCxt.getValue();
            Assert.assertTrue(value instanceof I64Value);
            Assert.assertEquals(((I64Value) value).getValue(), 66666666666L);

            instCxt.setValue(new I64Value(111L));

            value = instCxt.getValue();
            Assert.assertTrue(value instanceof I64Value);
            Assert.assertEquals(((I64Value) value).getValue(), 111L);
        }
    }

    @Test
    public void testGetV128() {
        String value = "-170141183460469231731687303715884105728";
        try(GlobalTypeContext typeCxt = new GlobalTypeContext(ValueType.i64, Mutability.VAR);
            GlobalInstanceContext instCxt = new GlobalInstanceContext(typeCxt, new V128Value(value))) {

            GlobalTypeContext globalTypeContext = instCxt.getGlobalType();
            Assert.assertEquals(globalTypeContext.getValueType(), typeCxt.getValueType());
            Assert.assertEquals(globalTypeContext.getMutability(), typeCxt.getMutability());
            Assert.assertTrue(instCxt.getValue() instanceof V128Value);
            Assert.assertEquals(((V128Value) instCxt.getValue()).getValue(), value);

        }
    }
}
