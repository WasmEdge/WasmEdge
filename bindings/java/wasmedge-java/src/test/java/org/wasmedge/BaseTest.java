package org.wasmedge;


import org.wasmedge.enums.ValueType;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.file.Paths;
import java.util.List;

public class BaseTest {
    protected static final String FIB_WASM_PATH = "apiTestData/fibonacci.wasm";
    protected static final String TEST_WASM_PATH = "apiTestData/test.wasm";
    protected static final String IMPORT_WASM_PATH = "apiTestData/import.wasm";
    protected static final String INVALID_WASM_PATH = "apiTestData/invalid_path.wasm";
    protected static final String FUNC_NAME = "fib";
    public static HostFunction extAdd = new HostFunction() {
        @Override
        public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
            WasmEdgeI32Value value = (WasmEdgeI32Value) params.get(1);
            returns.add(new WasmEdgeI32Value(value.getValue() + 1));
            return new Result();
        }
    };
    public static HostFunction extSub = new HostFunction() {
        @Override
        public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
            return new Result();
        }
    };
    public static HostFunction extMul = new HostFunction() {
        @Override
        public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
            return new Result();
        }
    };
    public static HostFunction extDiv = new HostFunction() {
        @Override
        public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
            return new Result();
        }
    };
    public static HostFunction extTerm = new HostFunction() {
        @Override
        public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
            return new Result();
        }
    };
    public static HostFunction extFail = new HostFunction() {
        @Override
        public Result apply(MemoryInstanceContext mem, List<WasmEdgeValue> params, List<WasmEdgeValue> returns) {
            return new Result();
        }
    };

    static {
        WasmEdge.init();
    }

    byte[] WASM_MAGIC = {0x00, 0x61, 0x73, 0x6D};

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

        try (FileInputStream in = new FileInputStream(new File(filePath))) {
            return in.readAllBytes();
        } catch (IOException ex) {
            throw new RuntimeException(ex);
        }

    }

    ImportObjectContext createExternModule(String name) {
        ImportObjectContext importObjectContext = new ImportObjectContext(name);

        ValueType[] params = new ValueType[] {ValueType.ExternRef, ValueType.i32};
        ValueType[] returns = new ValueType[] {ValueType.i32};

        FunctionTypeContext functionTypeContext = new FunctionTypeContext(params, returns);

        FunctionInstanceContext hostFunc = new FunctionInstanceContext(functionTypeContext,
                extAdd, null, 0);
        importObjectContext.addFunction("func-add", hostFunc);

        hostFunc = new FunctionInstanceContext(functionTypeContext,
                extSub, null, 0);
        importObjectContext.addFunction("func-sub", hostFunc);

        hostFunc = new FunctionInstanceContext(functionTypeContext,
                extMul, null, 0);
        importObjectContext.addFunction("func-mul", hostFunc);

        hostFunc = new FunctionInstanceContext(functionTypeContext,
                extDiv, null, 0);
        importObjectContext.addFunction("func-div", hostFunc);

        functionTypeContext.delete();

        functionTypeContext = new FunctionTypeContext(null, returns);

        hostFunc = new FunctionInstanceContext(functionTypeContext, extTerm, null, 0);
        importObjectContext.addFunction("func-term", hostFunc);

        hostFunc = new FunctionInstanceContext(functionTypeContext, extFail, null, 0);
        importObjectContext.addFunction("func-fail", hostFunc);

        functionTypeContext.delete();

        return importObjectContext;

    }

}
