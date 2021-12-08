package org.wasmedge;

public class WasmEdgeLimit {
    private boolean hasMax;
    private long min;
    private long max;

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
}
