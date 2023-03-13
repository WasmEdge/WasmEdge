package org.wasmedge;

import java.math.BigInteger;
import org.wasmedge.enums.ValueType;

/**
 * i128 value.
 */
public final class V128Value implements Value {
    public static final BigInteger V128_MAX =
        new BigInteger("170141183460469231731687303715884105727");
    public static final BigInteger V128_MIN =
        new BigInteger("-170141183460469231731687303715884105728");

    private String value;
    private BigInteger v128Value;

    /**
     * Construct a v128 value from string.
     *
     * @param value string form of v128 value, +/- and digits only.
     */
    public V128Value(String value) {
        BigInteger intValue = new BigInteger(value);
        if (intValue.compareTo(V128_MAX) > 0 || intValue.compareTo(V128_MIN) < 0) {
            throw new IllegalArgumentException("Value out of boundary.");
        }
        this.v128Value = intValue;
        this.value = value;
    }

    @Override
    public ValueType getType() {
        return ValueType.v128;
    }

    public String getValue() {
        return this.value;
    }

    public void setValue(String value) {
        this.value = value;
    }
}
