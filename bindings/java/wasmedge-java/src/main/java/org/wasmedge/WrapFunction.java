package org.wasmedge;

import java.util.List;

public interface WrapFunction {
    void apply(List<WasmEdgeValue> params, List<WasmEdgeValue> returns);
}
