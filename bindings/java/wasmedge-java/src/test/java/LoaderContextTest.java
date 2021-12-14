import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.wasmedge.ASTModuleContext;
import org.wasmedge.ConfigureContext;
import org.wasmedge.LoaderContext;

import java.io.FileInputStream;
import java.nio.Buffer;

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
        ASTModuleContext moduleContext = loaderContext.parseFromFile(TEST_WASM_PATH);
        moduleContext.delete();
    }


    @Test
    public void testParseFromInvalidFile() {
        ASTModuleContext moduleContext = loaderContext.parseFromFile(TEST_WASM_PATH);
        moduleContext.delete();
    }

    @Test
    public void testParseFromBuffer() throws Exception {
        byte[] buffer = new byte[1024];

        try(FileInputStream fin = new FileInputStream(TEST_WASM_PATH)) {
            int len = fin.read(buffer, 0, 1024);
            ASTModuleContext moduleContext = loaderContext.parseFromBuffer(buffer, len);
            moduleContext.delete();
        }
        ASTModuleContext moduleContext = loaderContext.parseFromFile(TEST_WASM_PATH);
        moduleContext.delete();
    }





}
