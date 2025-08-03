#include <wasmedge/wasmedge.h>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

int main() {
    // Check if model file exists
    const std::string modelPath = "./rwkv.cpp-169M.bin";
    if (!std::ifstream(modelPath)) {
        std::cerr << "Model file not found: " << modelPath << "\n";
        std::cerr << "Please place a valid RWKV model file at the specified path.\n";
        return 1;
    }

    // Initialize WasmEdge VM
    WasmEdge_ConfigureContext *Conf = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(Conf, WasmEdge_HostRegistration_Wasi);
    WasmEdge_VMContext *VM = WasmEdge_VMCreate(Conf, nullptr);
    assert(VM && "Failed to create VM");

    // Load WasmEdge plugin
    WasmEdge_Result Res = WasmEdge_VMLoadPlugin(VM, "/usr/local/lib/wasmedge/libwasmedgePluginRWKV.so");
    if (!WasmEdge_ResultOK(Res)) {
        std::cerr << "Failed to load plugin: " << WasmEdge_ResultGetMessage(Res) << "\n";
        return 1;
    }

    // Get RWKV module instance
    WasmEdge_String modName = WasmEdge_StringCreateByCString("wasmedge_rwkv");
    WasmEdge_ModuleInstanceContext *Mod = WasmEdge_VMGetImportModuleContext(VM, modName);
    assert(Mod != nullptr);

    // Call `load_model` with args
    WasmEdge_String funcName = WasmEdge_StringCreateByCString("load_model");
    std::string path = modelPath;
    uint32_t ptr = 0;
    uint32_t len = path.length();

    // Set model path in WASM memory
    WasmEdge_MemoryInstanceContext *MemCxt = WasmEdge_VMGetMemoryInstanceContext(VM, 0);
    assert(MemCxt != nullptr);
    Res = WasmEdge_MemoryInstanceSetData(MemCxt, (const uint8_t *)path.c_str(), ptr, len + 1);
    if (!WasmEdge_ResultOK(Res)) {
        std::cerr << "Failed to set model path in memory: " << WasmEdge_ResultGetMessage(Res) << "\n";
        return 1;
    }

    WasmEdge_Value params[4];
    WasmEdge_Value memPathPtr = WasmEdge_ValueGenI32(ptr);
    WasmEdge_Value memPathLen = WasmEdge_ValueGenI32(len);
    WasmEdge_Value threads = WasmEdge_ValueGenI32(1);
    WasmEdge_Value gpuLayers = WasmEdge_ValueGenI32(0);

    params[0] = memPathPtr;
    params[1] = memPathLen;
    params[2] = threads;
    params[3] = gpuLayers;

    // Fake WASM context: since we’re not running actual Wasm memory in this test,
    // this direct call won't actually pass string data. So this is more of a smoke test.

    WasmEdge_Result res = WasmEdge_VMExecuteRegistered(VM, modName, funcName, params, 4, nullptr, 0);
    if (!WasmEdge_ResultOK(res)) {
        std::cerr << "load_model failed: " << WasmEdge_ResultGetMessage(res) << "\n";
        return 1;
    }

    std::cout << "Model loaded successfully.\n";

    // Call `eval`
    WasmEdge_StringDelete(funcName);
    funcName = WasmEdge_StringCreateByCString("eval");
    WasmEdge_Value evalParam[1] = { WasmEdge_ValueGenI32(42) };
    WasmEdge_Value evalRet[1];

    res = WasmEdge_VMExecuteRegistered(VM, modName, funcName, evalParam, 1, evalRet, 1);
    if (!WasmEdge_ResultOK(res)) {
        std::cerr << "eval failed: " << WasmEdge_ResultGetMessage(res) << "\n";
        return 1;
    }

    int32_t result = WasmEdge_ValueGetI32(evalRet[0]);
    std::cout << "Eval returned: " << result << std::endl;

    // Cleanup
    WasmEdge_VMDelete(VM);
    WasmEdge_ConfigureDelete(Conf);
    WasmEdge_StringDelete(funcName);
    WasmEdge_StringDelete(modName);

    return 0;
}
