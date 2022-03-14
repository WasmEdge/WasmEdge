package org.wasmedge;

import java.util.List;

public interface HostFunction {
    void apply(List<WasmEdgeValue> params, List<WasmEdgeValue> returns);
}
