package org.wasmedge;

import org.junit.Test;

public class StatisticsContextTest extends BaseTest {
    @Test
    public void testCreation() {
        StatisticsContext statisticsContext = new StatisticsContext();
        long[] costTable = {
                0,0,
                10,
                11,
                12,
                12,
                0,0,0,0,0,0,
                20,
                21,
                22,
                0
        };

        statisticsContext.setCostTable(costTable);
        statisticsContext.setCostLimit(500000);


        //TODO run wasm function

        long cost = statisticsContext.getTotalCost();
        statisticsContext.delete();
    }
}
