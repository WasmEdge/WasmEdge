package org.wasmedge;

import org.wasmedge.enums.ValueType;

/**
 * Base interface for value.
 */
public interface Value {
    ValueType getType();
}
