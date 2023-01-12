package org.wasmedge;

import org.junit.After;
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
        loaderContext.delete();
    }

    @Test
    public void testParseFromFile() {
        AstModuleContext moduleContext = loaderContext.parseFromFile(getResourcePath(TEST_WASM_PATH));
        moduleContext.delete();
    }


    @Test(expected = Exception.class)
    public void testParseFromInvalidFile() {
        AstModuleContext
            moduleContext = loaderContext.parseFromFile(getCwd() + "/" + INVALID_WASM_PATH);
        moduleContext.delete();
    }

    @Test
    public void testParseFromBuffer() throws Exception {
        byte[] buffer = new byte[1024];

        try (FileInputStream fin = new FileInputStream(getResourcePath(TEST_WASM_PATH))) {
            int len = fin.read(buffer, 0, 1024);
            AstModuleContext moduleContext = loaderContext.parseFromBuffer(buffer, len);
            moduleContext.delete();
        }
    }
}
