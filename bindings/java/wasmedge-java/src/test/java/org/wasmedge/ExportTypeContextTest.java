package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;
import org.wasmedge.enums.ExternalType;
import org.wasmedge.enums.RefType;
import org.wasmedge.enums.ValueType;
import org.wasmedge.enums.Mutability;

import java.util.List;

public class ExportTypeContextTest extends BaseTest {
    @Test
    public void testCreation() {

    }

    @Test
    public void test() {

        String[] externalNames = {
                "func-1", "func-2", "func-3", "func-4", "func-add",
                "func-mul-2", "func-call-indirect", "func-host-add",
                "func-host-sub", "func-host-mul", "func-host-div", "tab-func",
                "tab-ext", "mem", "glob-mut-i32", "glob-const-f32"};
        ExternalType[] exportTypes = {
                ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION,
                ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION,
                ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.FUNCTION, ExternalType.TABLE, ExternalType.TABLE,
                ExternalType.MEMORY, ExternalType.GLOBAL, ExternalType.GLOBAL
        };

        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext moduleContext = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ExportTypeContext> exportList = moduleContext.listExports();

            Assert.assertEquals(exportList.size(), 16);

            for (int i = 0; i < exportList.size(); i++) {
                Assert.assertEquals(exportList.get(i).getExternalName(), externalNames[i]);
                Assert.assertEquals(exportList.get(i).getExternalType(), exportTypes[i]);
            }
        }

    }

    @Test
    public void testGetFunctionType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ExportTypeContext> expTypes = mod.listExports();
            Assert.assertEquals(expTypes.get(4).getFunctionType().getParameters().size(), 2);
            Assert.assertEquals(expTypes.get(4).getFunctionType().getReturns().size(), 1);
        }
    }

    @Test
    public void testGetTableType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ExportTypeContext> expTypes = mod.listExports();
            Assert.assertEquals(expTypes.get(12).getTableType().getRefType(), RefType.EXTERREF);
            Assert.assertEquals(expTypes.get(12).getTableType().getLimit(),
                new Limit(false, 10, 10));
        }
    }

    @Test
    public void testGetMemoryType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ExportTypeContext> expTypes = mod.listExports();
            Assert.assertEquals(expTypes.get(13).getMemoryType().getLimit(), new Limit(true, 1, 3));
        }
    }


    @Test
    public void testGetGlobalType() {
        try(LoaderContext loaderContext = new LoaderContext(null);
            AstModuleContext mod = loaderContext.parseFromFile(getResourcePath(IMPORT_WASM_PATH))) {

            List<ExportTypeContext> expTypes = mod.listExports();
            Assert.assertEquals(expTypes.get(15).getGlobalType().getValueType(), ValueType.f32);
            Assert.assertEquals(expTypes.get(15).getGlobalType().getMutability(), Mutability.CONST);
        }
    }
}
