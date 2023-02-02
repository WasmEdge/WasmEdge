package org.wasmedge.enums;

/**
 * Host registration enum.
 */
public enum HostRegistration {

    WasmEdge_HostRegistration_Wasi(0);

    private final int val;

    HostRegistration(int val) {
        this.val = val;
    }

    public int getVal() {
        return val;
    }
}
