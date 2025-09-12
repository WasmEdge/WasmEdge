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
    WasmEdge_PluginLoadFromPath("/usr/local/lib/wasmedge/libwasmedgePluginRWKV.so");

    // Define a simple WebAssembly module (in WAT format, converted to binary in test)
    const std::string watFile = "test_rwkv.wat";
    const std::string wasmCode = 
        "(module\n"
        "  (import \"wasmedge_rwkv\" \"load_model\" (func $load_model (param i32 i32 i32 i32)))\n"
        "  (import \"wasmedge_rwkv\" \"eval\" (func $eval (param i32) (result i32)))\n"
        "  (memory 1)\n"
        "  (export \"memory\" (memory 0))\n"
        "  (data (i32.const 0) \"" + modelPath + "\\00\")\n"
        "  (func $main (export \"main\")\n"
        "    (i32.const 0)\n"
        "    (i32.const " + std::to_string(modelPath.length()) + ")\n"
        "    (i32.const 1)\n"
        "    (i32.const 0)\n"
        "    (call $load_model)\n"
        "    (i32.const 42)\n"
        "    (call $eval)\n"
        "    drop\n"
        "  )\n"
        ")";

    // Write WAT to temporary file
    std::ofstream out(watFile);
    out << wasmCode;
    out.close();

    // Convert WAT to WASM
    std::string cmd = "wat2wasm " + watFile + " -o test_rwkv.wasm";
    if (std::system(cmd.c_str()) != 0) {
        std::cerr << "Failed to convert WAT to WASM\n";
        return 1;
    }

    // Run the WebAssembly module
    WasmEdge_Result Res = WasmEdge_VMRunWasmFromFile(VM, "test_rwkv.wasm", WasmEdge_StringCreateByCString("main"), nullptr, 0, nullptr, 0);
    if (!WasmEdge_ResultOK(Res)) {
        std::cerr << "WASM execution failed: " << WasmEdge_ResultGetMessage(Res) << "\n";
        return 1;
    }

    std::cout << "Model loaded and eval executed successfully.\n";

    // Cleanup
    WasmEdge_VMDelete(VM);
    WasmEdge_ConfigureDelete(Conf);

    // Remove temporary files
    std::remove(watFile.c_str());
    std::remove("test_rwkv.wasm");

    return 0;
}
