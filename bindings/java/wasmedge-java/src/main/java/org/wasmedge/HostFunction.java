package org.wasmedge;

import java.util.List;

public interface HostFunction {
    Result apply(MemoryInstanceContext mem, List<Value> params, List<Value> returns);
}
