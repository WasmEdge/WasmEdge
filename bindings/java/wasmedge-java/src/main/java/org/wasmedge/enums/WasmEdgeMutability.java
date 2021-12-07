package org.wasmedge.enums;

import java.util.Arrays;

public enum WasmEdgeMutability {
    CONST(0x00),
    VAR(0x01);

    private WasmEdgeMutability(int value) {
        this.value = value;
    }
    private int value;

    public int getValue() {
        return value;
    }

    public static WasmEdgeMutability parseMutability(int value) {
        return Arrays.stream(values()).filter(v -> v.value == value)
                .findAny().orElse(null);
    }
}
