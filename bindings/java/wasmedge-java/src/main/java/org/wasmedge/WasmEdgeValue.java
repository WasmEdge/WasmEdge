package org.wasmedge;

import org.wasmedge.enums.ValueType;

public interface WasmEdgeValue {
    ValueType getType();
}
