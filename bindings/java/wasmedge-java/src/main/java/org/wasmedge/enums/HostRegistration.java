package org.wasmedge.enums;

/**
 * Host registration enum.
 */
public enum HostRegistration {

    WasmEdge_HostRegistration_Wasi(0),
    WasmEdge_HostRegistration_WasmEdge_Process(1);

    private final int val;

    HostRegistration(int val) {
        this.val = val;
    }

    public int getVal() {
        return val;
    }
}
