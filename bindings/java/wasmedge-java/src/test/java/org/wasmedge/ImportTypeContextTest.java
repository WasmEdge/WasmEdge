package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ExternalType;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.WasmEdgeMutability;

import java.util.List;

public class ImportTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        LoaderContext loaderContext = new LoaderContext(null);

        ASTModuleContext astModuleContext = loaderContext.parseFromFile("import.wasm");

        Assert.assertNotEquals(astModuleContext, null);

    }

    @Test
    public void test() {
        LoaderContext loaderContext = new LoaderContext(null);

        ASTModuleContext mod = loaderContext.parseFromFile("apiTestData/import.wasm");

        List<ImportTypeContext> impTypes = mod.listImports();

        Assert.assertEquals(impTypes.size(), 14);

        String[][] imports= {
                {"func-add", "extern"}, {"func-sub", "extern"}, {"func-mul", "extern"},
                {"func-mul", "extern"}, {"func-div", "extern"}, {"func-term", "extern"},
                {"func-fail", "extern"}, {"global-i32", "dummy"}, {"global-i64", "dummy"},
                {"global-f32", "dummy"}, {"global-f64", "dummy"}, {"tab-func", "dummy"},
                {"tab-ext", "dummy"}, {"mem1", "dummy"}, {"mem2", "dummy"}
        };
        ExternalType[] types = {
                ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION,
                ExternalType.FUNCTION, ExternalType.FUNCTION,
                ExternalType.GLOBAL, ExternalType.GLOBAL, ExternalType.GLOBAL, ExternalType.GLOBAL,
                ExternalType.TABLE, ExternalType.TABLE,
                ExternalType.MEMORY, ExternalType.MEMORY
        };

        for (int i = 0; i < imports.length; i++) {
            Assert.assertEquals(impTypes.get(i).getExternalType(), types[i]);
            Assert.assertEquals(impTypes.get(i).getExternalName(), imports[i][0]);
            Assert.assertEquals(impTypes.get(i).getModuleName(), imports[i][1]);
        }
        loaderContext.delete();
        mod.delete();
    }
    @Test
    public void testGetFunctionType() {
        LoaderContext loaderContext = new LoaderContext(null);

        ASTModuleContext mod = loaderContext.parseFromFile("apiTestData/import.wasm");

        List<ImportTypeContext> impTypes = mod.listImports();
        Assert.assertEquals(impTypes.get(4).getFunctionType().getParameters().size(), 0);
        Assert.assertEquals(impTypes.get(4).getFunctionType().getReturns().size(), 1);
        loaderContext.delete();
        mod.delete();
    }

    @Test
    public void testGetTableType() {
        LoaderContext loaderContext = new LoaderContext(null);

        ASTModuleContext mod = loaderContext.parseFromFile("apiTestData/import.wasm");

        List<ImportTypeContext> impTypes = mod.listImports();
        Assert.assertEquals(impTypes.get(11).getTableType().getRefType(), RefType.EXTERREF);
        Assert.assertEquals(impTypes.get(11).getTableType().getLimit(), new WasmEdgeLimit(true, 10, 30));
        loaderContext.delete();
        mod.delete();
    }

    @Test
    public void testGetMemoryType() {
        LoaderContext loaderContext = new LoaderContext(null);

        ASTModuleContext mod = loaderContext.parseFromFile("apiTestData/import.wasm");

        List<ImportTypeContext> impTypes = mod.listImports();
        Assert.assertEquals(impTypes.get(13).getMemoryType().getLimit(), new WasmEdgeLimit(false, 2, 2));
        loaderContext.delete();
        mod.delete();
    }


    @Test
    public void testGetGlobalType() {
        LoaderContext loaderContext = new LoaderContext(null);

        ASTModuleContext mod = loaderContext.parseFromFile("apiTestData/import.wasm");

        List<ImportTypeContext> impTypes = mod.listImports();
        Assert.assertEquals(impTypes.get(7).getGlobalType().getValueType(), ValueType.i64);
        Assert.assertEquals(impTypes.get(7).getGlobalType().getMutability(), WasmEdgeMutability.CONST);
        loaderContext.delete();
        mod.delete();
    }

}
