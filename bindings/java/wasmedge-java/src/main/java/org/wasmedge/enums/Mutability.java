package org.wasmedge.enums;

import java.util.Arrays;

/**
 * Enum for mutability.
 */
public enum Mutability {
    CONST(0x00),
    VAR(0x01);

    private final int value;

    Mutability(int value) {
        this.value = value;
    }

    public static Mutability parseMutability(int value) {
        return Arrays.stream(values()).filter(v -> v.value == value)
                .findAny().orElse(null);
    }

    public int getValue() {
        return value;
    }
}
