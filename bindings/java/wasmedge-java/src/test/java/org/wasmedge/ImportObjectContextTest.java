package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.WasmEdgeMutability;

import java.lang.invoke.ConstantCallSite;

public class ImportObjectContextTest extends BaseTest {
    @Test
    public void testCreate() {
        ImportObjectContext importObjectContext = new ImportObjectContext("extern");
        Assert.assertNotNull(importObjectContext);
    }

    @Test
    public void testAddHostFunction() {
        Assert.fail("Not implemented");
    }

    @Test
    public void testAddHostFunctionForBinding() {
        Assert.fail("Not implemented");
    }

    @Test
    public void testAddHostTable() {
        WasmEdgeLimit limit = new WasmEdgeLimit(true, 10, 20);
        TableTypeContext tableType = new TableTypeContext(RefType.FUNCREF, limit);
        TableInstanceContext tabIns = new TableInstanceContext(tableType);
        ImportObjectContext impCxt = new ImportObjectContext("extern");
        impCxt.addTable("table", tabIns);
    }

    @Test
    public void testAddHostMemory() {
        WasmEdgeLimit limit = new WasmEdgeLimit(true, 1, 2);
        MemoryTypeContext memType = new MemoryTypeContext(limit);
        MemoryInstanceContext memIns = new MemoryInstanceContext(memType);
        ImportObjectContext impCxt = new ImportObjectContext("extern");
        impCxt.addMemory("memory", memIns);
    }

    @Test
    public void testAddHostGlobal() {
        GlobalTypeContext glbType = new GlobalTypeContext(ValueType.i32, WasmEdgeMutability.CONST);
        GlobalInstanceContext glbIns = new GlobalInstanceContext(glbType, new WasmEdgeI32Value(666));
        ImportObjectContext impCxt = new ImportObjectContext("extern");
        impCxt.addGlobal("global_i32", glbIns);
    }

    @Test
    public void testCreateWASI() {
        Assert.fail("Not implemented");
    }

    @Test
    public void testInitWasiInVM() {
        Assert.fail("Not implemented");
    }

    @Test
    public void testCreateWasmEdgeProcess() {
        Assert.fail("Not implemented");
    }

    @Test
    public void testInitWasmEdgeProcessInVM() {
        Assert.fail("Not implemented");
    }

}
