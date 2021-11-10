package org.wasmedge;

public class WasmEdgeResult {
    private int code;

    public WasmEdgeResult(int code) {
        this.code = code;
    }

    public int getCode() {
        return this.code;
    }

}
