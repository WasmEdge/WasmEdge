package org.wasmedge;

import java.util.List;

public interface HostFunction {
    Result apply(Object data, MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns);
}
