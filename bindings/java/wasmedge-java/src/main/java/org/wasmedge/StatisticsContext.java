package org.wasmedge;

/**
 * Static context for vm execution.
 */
public class StatisticsContext extends NativeResource {

    public StatisticsContext() {
        super(0);
        nativeInit();
    }

    private StatisticsContext(long pointer) {
        super(pointer);
    }

    private native void nativeInit();

    public native int getInstrCount();

    public native double getInstrPerSecond();

    public native void setCostTable(long[] costTable);

    public native void setCostLimit(long costLimit);

    public native long getTotalCost();

    public native void close();

}
