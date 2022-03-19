package org.wasmedge;

import org.junit.Assert;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class ExecutorContextTest extends BaseTest {
    @Test
    public void testCreation() {

    }

    @Test
    public void testExecutorWithStatistics() {
        ConfigureContext configureContext = new ConfigureContext();

        configureContext.setStatisticsSetInstructionCounting(true);
        configureContext.setStatisticsSetCostMeasuring(true);
        configureContext.setStatisticsSetTimeMeasuring(true);

        ASTModuleContext astModuleContext = loadMode(configureContext, TEST_WASM_PATH);
        ValidatorContext validatorContext = new ValidatorContext(configureContext);
        validatorContext.validate(astModuleContext);
    }

    @Test
    public void testRegisterWasmModule() {
        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();
        ASTModuleContext mod = loadMode(configureContext, FIB_WASM_PATH);
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        String modName = "extern";
        executorContext.registerModule(storeContext, mod, modName);
    }

    @Test(expected = Exception.class)
    public void testRegisterWasmModuleNameConflict() {
        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();
        ASTModuleContext mod = loadMode(configureContext, FIB_WASM_PATH);
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();

        String modName2 = "extern";
        executorContext.registerModule(storeContext, mod, modName2);

        ASTModuleContext mod2 = loadMode(configureContext, FIB_WASM_PATH);
        executorContext.registerModule(storeContext, mod2, modName2);
    }

    @Test
    public void testInstantiateModule() {
        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();
        ASTModuleContext mod = loadMode(configureContext, TEST_WASM_PATH);
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        executorContext.instantiate(storeContext, mod);
    }

    @Test
    public void testInstantiateModuleWithNullStore() {
        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();
        ASTModuleContext mod = loadMode(configureContext, TEST_WASM_PATH);
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        executorContext.instantiate(null, mod);
    }

    @Test
    public void testInstantiateModuleWithNullMod() {
        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        executorContext.instantiate(storeContext, null);
    }

    @Test
    public void testOverrideInstantiatedModule() {
        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();
        ASTModuleContext mod = loadMode(configureContext, TEST_WASM_PATH);
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        executorContext.instantiate(storeContext, mod);
        executorContext.instantiate(storeContext, mod);
    }

    @Test
    public void testInvokeFunction() {
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));
        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();

        LoaderContext loaderContext = new LoaderContext(null);
        ASTModuleContext moduleContext = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        executorContext.instantiate(storeContext, moduleContext);
        executorContext.invoke(storeContext, FUNC_NAME, params, returns);
    }

    @Test(expected = Exception.class)
    public void testInvokeFunctionParamMismatch() {
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));
        params.add(new WasmEdgeI32Value(3));
        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();

        LoaderContext loaderContext = new LoaderContext(null);
        ASTModuleContext moduleContext = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        executorContext.instantiate(storeContext, moduleContext);
        executorContext.invoke(storeContext, FUNC_NAME, params, returns);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvokeFunctionNullParam() {
        String funcName = "func-mul-2";
        List<WasmEdgeValue> returns = new ArrayList<>();
        ExecutorContext executorContext = new ExecutorContext(new ConfigureContext(), new StatisticsContext());
        executorContext.invoke(new StoreContext(), funcName, null, returns);
    }

    @Test(expected = Exception.class)
    public void testInvokeFunctionFunctionNotFound() {
        List<WasmEdgeValue> params = new ArrayList<>();
        params.add(new WasmEdgeI32Value(3));
        List<WasmEdgeValue> returns = new ArrayList<>();
        returns.add(new WasmEdgeI32Value());

        ConfigureContext configureContext = new ConfigureContext();
        StatisticsContext statisticsContext = new StatisticsContext();

        LoaderContext loaderContext = new LoaderContext(null);
        ASTModuleContext moduleContext = loaderContext.parseFromFile(getResourcePath(FIB_WASM_PATH));
        ExecutorContext executorContext = new ExecutorContext(configureContext, statisticsContext);
        StoreContext storeContext = new StoreContext();
        executorContext.instantiate(storeContext, moduleContext);
        executorContext.invoke(storeContext, FUNC_NAME + UUID.randomUUID(), params, returns);
    }


    @Test
    public void testRegisterImport() {
        ExecutorContext exeCxt = new ExecutorContext(null, null);
        ImportObjectContext impCxt = new ImportObjectContext("ext");
        StoreContext storeCxt = new StoreContext();
        exeCxt.registerImport(storeCxt, impCxt);
    }
}
