package org.wasmedge.enums;

import java.util.Arrays;

/**
 * Enum for ref types.
 */
public enum RefType {

    FUNCREF(0x70),
    EXTERREF(0x6F);
    private final int val;

    /**
     * Create a ref type.
     *
     * @param val value for type.
     */
    RefType(int val) {
        this.val = val;
    }

    /**
     * Get type for a given val.
     *
     * @param val value.
     * @return type for this value.
     */
    public static RefType getType(int val) {
        return Arrays.stream(values())
                .filter(type -> type.val == val)
                .findAny().orElse(null);
    }

    public int getVal() {
        return this.val;
    }
}
