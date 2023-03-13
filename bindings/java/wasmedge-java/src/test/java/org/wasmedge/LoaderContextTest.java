package org.wasmedge;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.io.FileInputStream;

public class LoaderContextTest extends BaseTest {
    private LoaderContext loaderContext;

    @Before
    public void setUp() {
        loaderContext = new LoaderContext(null);
    }

    @After
    public void tearDown() {
        loaderContext.close();
    }

    @Test
    public void testParseFromFile() {
        try(AstModuleContext moduleContext = loaderContext.parseFromFile(getResourcePath(TEST_WASM_PATH))) {
            Assert.assertNotNull(moduleContext);
        }
    }


    @Test(expected = Exception.class)
    public void testParseFromInvalidFile() {
        try(AstModuleContext moduleContext = loaderContext.parseFromFile(getCwd() + "/" + INVALID_WASM_PATH)) {
            Assert.fail();
        }
    }

    @Test
    public void testParseFromBuffer() throws Exception {
        byte[] buffer = new byte[1024];

        try (FileInputStream fin = new FileInputStream(getResourcePath(TEST_WASM_PATH))) {
            int len = fin.read(buffer, 0, 1024);
            try(AstModuleContext moduleContext = loaderContext.parseFromBuffer(buffer, len)) {
                Assert.assertNotNull(moduleContext);
            }
        }
    }
}
