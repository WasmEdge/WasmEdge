package org.wasmedge.enums;

import java.util.Arrays;

public enum RefType {

    FUNCREF(0x70),
    EXTERREF(0x6F);
    private final int val;

    RefType(int val) {
        this.val = val;
    }

    public static RefType getType(int val) {
        return Arrays.stream(values())
                .filter(type -> type.val == val)
                .findAny().orElse(null);
    }

    public int getVal() {
        return this.val;
    }
}
