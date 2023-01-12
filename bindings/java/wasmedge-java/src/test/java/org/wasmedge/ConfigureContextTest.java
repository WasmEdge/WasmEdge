package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.CompilerOptimizationLevel;
import org.wasmedge.enums.CompilerOutputFormat;
import org.wasmedge.enums.HostRegistration;
import org.wasmedge.enums.Proposal;

public class ConfigureContextTest extends BaseTest {

    @Test
    public void testProposal() {
        ConfigureContext context = new ConfigureContext();
        context.addProposal(Proposal.WasmEdge_Proposal_SIMD);
        context.removeProposal(Proposal.WasmEdge_Proposal_ReferenceTypes);
        Assert.assertTrue(context.hasProposal(Proposal.WasmEdge_Proposal_SIMD));
        Assert.assertFalse(context.hasProposal(Proposal.WasmEdge_Proposal_ReferenceTypes));
    }

    @Test
    public void testHostRegistration() {
        ConfigureContext context = new ConfigureContext();
        context.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        Assert.assertTrue(context.hasHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi));
        context.removeHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        Assert.assertFalse(context.hasHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi));
    }

    @Test
    public void testMaxMemoryPage() {
        ConfigureContext context = new ConfigureContext();
        long maxPage = 1024;
        context.setMaxMemoryPage(maxPage);
        Assert.assertEquals(maxPage, context.getMaxMemoryPage());
    }

    @Test
    public void testCompilerOptimizationLevel() {
        ConfigureContext context = new ConfigureContext();
        CompilerOptimizationLevel level = CompilerOptimizationLevel.WasmEdge_CompilerOptimizationLevel_O0;
        context.setCompilerOptimizationLevel(level);
        Assert.assertEquals(context.getCompilerOptimizationLevel(), level);
    }

    @Test
    public void testCompilerOutputFormat() {
        ConfigureContext context = new ConfigureContext();
        CompilerOutputFormat outputFormat = CompilerOutputFormat.WasmEdge_CompilerOutputFormat_Wasm;
        context.setCompilerOutputFormat(outputFormat);
        Assert.assertEquals(context.getCompilerOutputFormat(), outputFormat);
    }

    @Test
    public void testCompilerIsDumpIR() {
        ConfigureContext context = new ConfigureContext();
        boolean isDumpIR = true;
        context.setCompilerIsDumpIr(isDumpIR);
        Assert.assertEquals(context.getCompilerIsDumpIr(), isDumpIR);
    }

    @Test
    public void testCompilerIsGenericBinary() {
        ConfigureContext context = new ConfigureContext();
        boolean isGenericBinary = true;
        context.setCompilerIsGenericBinary(isGenericBinary);
        Assert.assertEquals(context.getCompilerIsGenericBinary(), isGenericBinary);
    }

    @Test
    public void testIsInstructionCounting() {
        ConfigureContext context = new ConfigureContext();
        boolean isInstructionCounting = true;
        context.setStatisticsSetInstructionCounting(isInstructionCounting);
        Assert.assertEquals(context.isStatisticsSetInstructionCounting(), isInstructionCounting);
    }

    @Test
    public void testCompilerCostMeasuring() {
        ConfigureContext context = new ConfigureContext();
        boolean costMeasuring = true;
        context.setStatisticsSetCostMeasuring(costMeasuring);
        Assert.assertEquals(context.isStatisticsSetCostMeasuring(), costMeasuring);
    }

    @Test
    public void testTimeMeasuring() {
        ConfigureContext context = new ConfigureContext();
        boolean timeMeasuring = true;
        context.setStatisticsSetTimeMeasuring(timeMeasuring);
        Assert.assertEquals(context.isStatisticsSetTimeMeasuring(), timeMeasuring);
    }

}
