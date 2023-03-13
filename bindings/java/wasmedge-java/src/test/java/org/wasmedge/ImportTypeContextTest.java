package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ExternalType;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

import java.util.List;

public class ImportTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext astModuleContext = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {
            Assert.assertNotEquals(astModuleContext, null);
        }
    }

    @Test
    public void test() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ImportTypeContext> impTypes = mod.listImports();

            Assert.assertEquals(impTypes.size(), 14);

            String[][] imports = {
                {"func-add", "extern"}, {"func-sub", "extern"}, {"func-mul", "extern"},
                {"func-div", "extern"}, {"func-term", "extern"},
                {"func-fail", "extern"}, {"glob-i32", "dummy"}, {"glob-i64", "dummy"},
                {"glob-f32", "dummy"}, {"glob-f64", "dummy"}, {"tab-func", "dummy"},
                {"tab-ext", "dummy"}, {"mem1", "dummy"}, {"mem2", "dummy"}
            };
            ExternalType[] types = {
                ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION,
                ExternalType.FUNCTION, ExternalType.FUNCTION,
                ExternalType.FUNCTION, ExternalType.GLOBAL, ExternalType.GLOBAL,
                ExternalType.GLOBAL,
                ExternalType.GLOBAL, ExternalType.TABLE,
                ExternalType.TABLE, ExternalType.MEMORY, ExternalType.MEMORY

            };

            for (int i = 0; i < imports.length; i++) {
                Assert.assertEquals(impTypes.get(i).getExternalType(), types[i]);
                Assert.assertEquals(impTypes.get(i).getExternalName(), imports[i][0]);
                Assert.assertEquals(impTypes.get(i).getModuleName(), imports[i][1]);
            }
        }
    }

    @Test
    public void testGetFunctionType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ImportTypeContext> impTypes = mod.listImports();
            Assert.assertEquals(impTypes.get(4).getFunctionType().getParameters().size(), 0);
            Assert.assertEquals(impTypes.get(4).getFunctionType().getReturns().size(), 1);
        }
    }

    @Test
    public void testGetTableType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ImportTypeContext> impTypes = mod.listImports();
            Assert.assertEquals(impTypes.get(11).getTableType().getRefType(), RefType.EXTERREF);
            Assert.assertEquals(impTypes.get(11).getTableType().getLimit(),
                new Limit(true, 10, 30));
        }
    }

    @Test
    public void testGetMemoryType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ImportTypeContext> impTypes = mod.listImports();
            Assert.assertEquals(impTypes.get(13).getMemoryType().getLimit(),
                new Limit(false, 2, 2));
        }
    }

    @Test
    public void testGetGlobalType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ImportTypeContext> impTypes = mod.listImports();
            Assert.assertEquals(impTypes.get(7).getGlobalType().getValueType(), ValueType.i64);
            Assert.assertEquals(impTypes.get(7).getGlobalType().getMutability(), Mutability.CONST);
        }
    }
}
