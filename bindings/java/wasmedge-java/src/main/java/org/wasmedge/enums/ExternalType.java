package org.wasmedge.enums;

import java.util.Arrays;

/**
 * Enum for external types.
 */
public enum ExternalType {
    FUNCTION(0x00),
    TABLE(0x01),
    MEMORY(0x02),
    GLOBAL(0x03);

    private final int val;

    ExternalType(int val) {
        this.val = val;
    }

    public static ExternalType getByValue(int val) {
        return Arrays.stream(values()).filter(type -> type.val == val)
                .findAny().orElse(null);
    }

}
