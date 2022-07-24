package org.wasmedge;

import java.util.List;

public interface HostFunction {
    Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns);
}
