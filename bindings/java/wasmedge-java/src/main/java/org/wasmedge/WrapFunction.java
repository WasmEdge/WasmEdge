package org.wasmedge;

import java.util.List;

/**
 * Base interface for wrap function.
 */
public interface WrapFunction {
    void apply(List<Value> params, List<Value> returns);
}
