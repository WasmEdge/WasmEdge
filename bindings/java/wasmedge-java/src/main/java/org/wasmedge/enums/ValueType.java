package org.wasmedge.enums;

import java.util.Arrays;

public enum ValueType {
    i32(0x7F),
    i64(0x7E),
    f32(0x7D),
    f64(0x7C),
    v128(0x7B),
    FuncRef(0x70),
    ExternRef(0x6F);
    private int value;

    private ValueType(int value) {
        this.value = value;
    }
    public int getValue() {
        return value;
    }

    public static ValueType parseType(int value) {
        return Arrays.stream(values()).filter(v -> v.value == value)
                .findAny()
                .orElse(null);
    }
}
