package org.wasmedge;


import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Paths;

public class BaseTest {
    protected static final String FIB_WASM_PATH = "apiTestData/fibonacci.wasm";
    protected static final String TEST_WASM_PATH = "apiTestData/test.wasm";
    protected static final String IMPORT_WASM_PATH = "apiTestData/import.wasm";
    protected static final String INVALID_WASM_PATH = "apiTestData/invalid_path.wasm";
    protected static final String FUNC_NAME = "fib";
    byte[] WASM_MAGIC = {0x00, 0x61, 0x73, 0x6D};

    static  {
        WasmEdge.init();
    }

    public static ASTModuleContext loadMode(ConfigureContext configureContext, String path) {
        LoaderContext loaderContext = new LoaderContext(configureContext);
        ASTModuleContext astModuleContext = loaderContext.parseFromFile(getResourcePath(path));
        loaderContext.delete();
        return astModuleContext;
    }

    public static String getResourcePath(String path) {
        try {
            URL resource = BaseTest.class.getClassLoader().getResource(path);

            return Paths.get(resource.toURI()).toFile().getAbsolutePath();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
    public static String getCwd() {
        return getResourcePath("./");
    }

    public byte[] loadFile(String filePath) {

        try(FileInputStream in = new FileInputStream(new File(filePath))) {
            return in.readAllBytes();
        } catch (IOException ex) {
            throw new RuntimeException(ex);
        }

    }

}
