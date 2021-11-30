package org.wasmedge;

import org.wasmedge.enums.CompilerOptimizationLevel;
import org.wasmedge.enums.CompilerOutputFormat;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.Proposal;

public class ConfigureContext {
    private long pointer;

    public ConfigureContext() {
        nativeInit();
    }

    private native void nativeInit();
    private native void delete();

    public native void addProposal(Proposal proposal);

    public native void removeProposal(Proposal proposal);

    public native boolean hasProposal(Proposal proposal);

    public native void addHostRegistration(HostRegistration hostRegistration);

    public native void removeHostRegistration(HostRegistration hostRegistration);

    public native boolean hasHostRegistration(HostRegistration hostRegistration);

    public native void setMaxMemoryPage(long pages);

    public native long getMaxMemoryPage();

    public native void setCompilerOptimizationLevel(CompilerOptimizationLevel optimizationLevel);

    public native CompilerOptimizationLevel getCompilerOptimizationLevel();

    public native void setCompilerOutputFormat(CompilerOutputFormat compilerOutputFormat);

    public native CompilerOutputFormat getCompilerOutputFormat();

    public native void setCompilerIsDumpIR(boolean isDumpIR);

    public native boolean getCompilerIsDumpIR();

    public native void setCompilerIsGenericBinary(boolean isGenericBinary);

    public native boolean getCompilerIsGenericBinary();

    public native void setStatisticsSetInstructionCounting(boolean statisticsSetInstructionCounting);
    public native boolean isStatisticsSetInstructionCounting();

    public native void setStatisticsSetCostMeasuring(boolean statisticsSetCostMeasuring);

    public native boolean isStatisticsSetCostMeasuring();

    public native void setStatisticsSetTimeMeasuring(boolean statisticsSetTimeMeasuring);

    public native boolean isStatisticsSetTimeMeasuring();

    public void destroy() {
        delete();
        pointer = 0;
    }

}
