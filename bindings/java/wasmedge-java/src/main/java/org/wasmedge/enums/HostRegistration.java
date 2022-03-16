package org.wasmedge.enums;

public enum HostRegistration {

    WasmEdge_HostRegistration_Wasi(0),
    WasmEdge_HostRegistration_WasmEdge_Process(1);

    private int val;
    private HostRegistration(int val) {
        this.val = val;
    }

    public int getVal() {
        return val;
    }
}
