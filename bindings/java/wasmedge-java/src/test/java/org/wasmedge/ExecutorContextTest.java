package org.wasmedge;

import org.junit.Test;

public class ExecutorContextTest extends BaseTest {
    @Test
    public void testCreation() {
        ExecutorContextTest executorContext = new ExecutorContextTest();
    }

    @Test
    public void testExecutorWithStatistics() {
        ConfigureContext configureContext = new ConfigureContext();

        configureContext.setStatisticsSetInstructionCounting(true);
        configureContext.setStatisticsSetCostMeasuring(true);
        configureContext.setStatisticsSetTimeMeasuring(true);

        ASTModuleContext astModuleContext = loadMode(configureContext);
        ValidatorContext validatorContext = new ValidatorContext(configureContext);
        validatorContext.validate(astModuleContext);
    }
}
