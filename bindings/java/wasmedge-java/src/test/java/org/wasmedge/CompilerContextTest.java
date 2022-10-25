package org.wasmedge;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.wasmedge.enums.CompilerOutputFormat;

import java.io.FileInputStream;

public class CompilerContextTest extends BaseTest {
    private CompilerContext target;

    @Before
    public void setUp() {
        target = new CompilerContext(new ConfigureContext());
    }

    @After
    public void tearDown() {
        target.delete();
    }

    @Test
    public void testCompile() throws Exception {
        String path = "test_aot.wasm";
        target.compile(getResourcePath(TEST_WASM_PATH), getResourcePath("./") + path);
        byte[] buf = new byte[4];

        try (FileInputStream fin = new FileInputStream(getResourcePath("./") + path)) {
            fin.read(buf, 0, 4);
        }

        Assert.assertArrayEquals(buf, WASM_MAGIC);
    }

    @Test(expected = Exception.class)
    public void testInvalidPath() {
        target.compile("invalid_path.wasm", "invalid_aot.wasm");
    }

    @Test
    public void testCompileNative() throws Exception {
        ConfigureContext config = new ConfigureContext();
        config.setCompilerOutputFormat(CompilerOutputFormat.WasmEdge_CompilerOutputFormat_Native);
        target = new CompilerContext(config);
        String path = getCwd() + "test_aot.wasm";
        target.compile(getResourcePath(TEST_WASM_PATH), path);
        byte[] buf = new byte[4];

        try (FileInputStream fin = new FileInputStream(path)) {
            fin.read(buf, 0, 4);
        }

    }

}
