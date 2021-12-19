package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

public class WasmEdgeTest extends BaseTest {
    @Test
    public void testVersion() {
        WasmEdge wasmEdge = new WasmEdge();
        Assert.assertNotNull(wasmEdge.getVersion());
        Assert.assertTrue(wasmEdge.getMajorVersion() >= 0);
        Assert.assertTrue(wasmEdge.getMinorVersion() >= 0);
        Assert.assertTrue(wasmEdge.getPatchVersion() >= 0);
    }

    @Test
    public void testSetLogLevel() {
        WasmEdge wasmEdge = new WasmEdge();
        wasmEdge.setLogLevel(WasmEdge.LogLevel.DEBUG);
    }
}
