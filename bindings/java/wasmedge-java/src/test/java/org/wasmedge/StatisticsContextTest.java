package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.HostRegistration;

import java.util.ArrayList;
import java.util.List;

public class StatisticsContextTest extends BaseTest {
    @Test
    public void testCreation() {
        ConfigureContext configureContext = new ConfigureContext();

        configureContext.setStatisticsSetInstructionCounting(true);
        configureContext.setStatisticsSetCostMeasuring(true);
        configureContext.setStatisticsSetTimeMeasuring(true);

        configureContext.addHostRegistration(HostRegistration.WasmEdge_HostRegistration_Wasi);
        WasmEdgeVm vm = new WasmEdgeVm(configureContext, new StoreContext());

        StatisticsContext statisticsContext = vm.getStatisticsContext();

        Assert.assertNotNull(statisticsContext);
        long[] costTable = {
                0, 0,
                10,
                11,
                12,
                12,
                0, 0, 0, 0, 0, 0,
                20,
                21,
                22,
                0
        };
        statisticsContext.setCostTable(costTable);
        statisticsContext.setCostLimit(500000);

        List<Value> params = new ArrayList<>();
        params.add(new I32Value(3));

        List<Value> returns = new ArrayList<>();
        returns.add(new I32Value());

        vm.runWasmFromFile(getResourcePath(FIB_WASM_PATH), FUNC_NAME, params, returns);
        Assert.assertEquals(3, ((I32Value) returns.get(0)).getValue());

        long cost = statisticsContext.getTotalCost();
        double ips = statisticsContext.getInstrPerSecond();
        int instrCnt = statisticsContext.getInstrCount();

        Assert.assertTrue(cost > 0);
        Assert.assertTrue(ips > 0);
        Assert.assertTrue(instrCnt > 0);

//        statisticsContext.delete();
        vm.destroy();
    }
}
