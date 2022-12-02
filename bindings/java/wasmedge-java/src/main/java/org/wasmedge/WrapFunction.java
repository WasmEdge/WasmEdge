package org.wasmedge;

import java.util.List;

public interface WrapFunction {
    void apply(List<Value> params, List<Value> returns);
}
