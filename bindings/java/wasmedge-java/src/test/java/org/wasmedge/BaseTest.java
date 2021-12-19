package org.wasmedge;


import java.net.URL;
import java.nio.file.Paths;

public class BaseTest {
    protected static final String WASM_PATH = "apiTestData/fibonacci.wasm";
    protected static final String TEST_WASM_PATH = "apiTestData/test.wasm";
    protected static final String IMPORT_WASM_PATH = "apiTestData/import.wasm";
    protected static final String FUNC_NAME = "fib";
    byte[] WASM_MAGIC = {0x00, 0x61, 0x73, 0x60};

    static  {
        System.out.println("init native lib");
        WasmEdge.init();
        System.out.println("done init native lib ");
    }

    public static ASTModuleContext loadMode(ConfigureContext configureContext) {
        LoaderContext loaderContext = new LoaderContext(configureContext);
        ASTModuleContext astModuleContext = loaderContext.parseFromFile(TEST_WASM_PATH);
        loaderContext.delete();
        return astModuleContext;
    }

    public static String getResourcePath(String path) {
        try {
            URL resource = BaseTest.class.getClassLoader().getResource(WASM_PATH);

            return Paths.get(resource.toURI()).toFile().getAbsolutePath();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

}
