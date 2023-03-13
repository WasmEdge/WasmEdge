package org.wasmedge;

import org.wasmedge.enums.CompilerOptimizationLevel;
import org.wasmedge.enums.CompilerOutputFormat;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.Proposal;

/**
 * Configure Context for WasmEdge VM.
 */
public class ConfigureContext extends NativeResource {

    public ConfigureContext() {
        super();
        nativeInit();
    }

    private native void nativeInit();

    public native void close();

    public void addProposal(Proposal proposal) {
        addProposal(proposal.ordinal());
    }

    private native void addProposal(int proposal);

    public void removeProposal(Proposal proposal) {
        removeProposal(proposal.ordinal());
    }

    private native void removeProposal(int proposal);

    public boolean hasProposal(Proposal proposal) {
        return hasProposal(proposal.ordinal());
    }

    private native boolean hasProposal(int proposal);

    public void addHostRegistration(HostRegistration hostRegistration) {
        addHostRegistration(hostRegistration.ordinal());
    }

    private native void addHostRegistration(int hostRegistration);

    public void removeHostRegistration(HostRegistration hostRegistration) {
        removeHostRegistration(hostRegistration.ordinal());
    }

    private native void removeHostRegistration(int hostRegistration);

    public boolean hasHostRegistration(HostRegistration hostRegistration) {
        return hasHostRegistration(hostRegistration.ordinal());
    }

    private native boolean hasHostRegistration(int hostRegistration);

    public native long getMaxMemoryPage();

    public native void setMaxMemoryPage(long pages);

    public CompilerOptimizationLevel getCompilerOptimizationLevel() {
        return CompilerOptimizationLevel.values()[nativeGetCompilerOptimizationLevel()];
    }

    public void setCompilerOptimizationLevel(CompilerOptimizationLevel optimizationLevel) {
        setCompilerOptimizationLevel(optimizationLevel.ordinal());
    }

    private native void setCompilerOptimizationLevel(int optimizationLevel);

    private native int nativeGetCompilerOptimizationLevel();

    public CompilerOutputFormat getCompilerOutputFormat() {
        return CompilerOutputFormat.values()[nativeGetCompilerOutputFormat()];
    }

    public void setCompilerOutputFormat(CompilerOutputFormat compilerOutputFormat) {
        setCompilerOutputFormat(compilerOutputFormat.ordinal());
    }

    private native void setCompilerOutputFormat(int compilerOutputFormat);

    private native int nativeGetCompilerOutputFormat();

    public native boolean getCompilerIsDumpIr();

    public native void setCompilerIsDumpIr(boolean isDumpIr);

    public native boolean getCompilerIsGenericBinary();

    public native void setCompilerIsGenericBinary(boolean isGenericBinary);

    public native boolean isStatisticsSetInstructionCounting();

    public native void setStatisticsSetInstructionCounting(
        boolean statisticsSetInstructionCounting);

    public native boolean isStatisticsSetCostMeasuring();

    public native void setStatisticsSetCostMeasuring(boolean statisticsSetCostMeasuring);

    public native boolean isStatisticsSetTimeMeasuring();

    public native void setStatisticsSetTimeMeasuring(boolean statisticsSetTimeMeasuring);

}
