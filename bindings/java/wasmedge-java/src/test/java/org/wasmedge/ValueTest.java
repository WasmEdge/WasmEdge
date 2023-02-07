package org.wasmedge;

import java.math.BigInteger;
import org.junit.Assert;
import org.junit.Test;

public class ValueTest extends BaseTest {

    @Test
    public void testV128Value() {
        String strVal = "123434";
        V128Value value = new V128Value(strVal);

        Assert.assertTrue(value.getValue().equals(strVal));
    }

    @Test
    public void testInvalidV128Value() {
        String invalidValue = V128Value.V128_MAX.add(BigInteger.ONE).toString();
        Assert.assertThrows(IllegalArgumentException.class, ()-> {
           V128Value value = new V128Value(invalidValue);
        });
    }

}
