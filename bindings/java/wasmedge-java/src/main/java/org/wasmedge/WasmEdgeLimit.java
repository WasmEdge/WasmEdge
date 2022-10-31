package org.wasmedge;

public class WasmEdgeLimit {
    private final boolean hasMax;
    private final long min;
    private final long max;

    public WasmEdgeLimit(boolean hasMax, long min, long max) {
        this.hasMax = hasMax;
        this.min = min;
        this.max = max;
    }

    public boolean isHasMax() {
        return hasMax;
    }

    public long getMin() {
        return min;
    }

    public long getMax() {
        return max;
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof WasmEdgeLimit) {
            WasmEdgeLimit that = (WasmEdgeLimit) other;
            return this.hasMax == that.hasMax
                    && this.min == that.min
                    && (!this.hasMax || (this.max == that.max));
        }
        return false;
    }
}
