package org.wasmedge;

import org.wasmedge.enums.WasmEdgeMutability;

public class GlobalInstanceContext {

    public GlobalInstanceContext(GlobalTypeContext typeCxt,
                                 WasmEdgeValue value) {

    }

    public native GlobalTypeContext getGlobalType();

    public native void setValue(WasmEdgeValue value);

    public native WasmEdgeValue getValue();

    public native void delete();
}
