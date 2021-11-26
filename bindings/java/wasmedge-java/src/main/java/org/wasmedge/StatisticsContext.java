package org.wasmedge;

public class StatisticsContext {
    private long pointer;

    public StatisticsContext() {

    }

    private native void nativeInit();

    public native int getInstrCount();

    public native double getInstrPerSecond();

    public native void setCostTable(long[] costTable);

    public native long getTotalCost();

    public void destroy() {
        delete();
        pointer = 0;
    }

    public native void delete();

}
