package org.wasmedge;

/**
 * Static context for vm execution.
 */
public class StatisticsContext {
    private long pointer;

    public StatisticsContext() {
        nativeInit();
    }

    private StatisticsContext(long pointer) {
        this.pointer = pointer;
    }

    private native void nativeInit();

    public native int getInstrCount();

    public native double getInstrPerSecond();

    public native void setCostTable(long[] costTable);

    public native void setCostLimit(long costLimit);

    public native long getTotalCost();

    public void destroy() {
        delete();
        pointer = 0;
    }

    public native void delete();

}
