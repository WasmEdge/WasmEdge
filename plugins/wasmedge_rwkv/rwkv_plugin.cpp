#include <wasmedge/wasmedge.h>
#include "rwkv.h"
#include <vector>
#include <string>

// Global RWKV context pointer
static struct rwkv_context *global_ctx = nullptr;

// Host function: load_model(path: string, threads: i32, gpu_layers: i32) -> void
// Loads an RWKV model from a file path with specified threads and GPU layers
extern "C" WasmEdge_Result RWKV_LoadModel(void *, const WasmEdge_CallingFrameContext *CallFrameCxt, const WasmEdge_Value *In, WasmEdge_Value *Out) {
    // Get memory instance
    WasmEdge_MemoryInstanceContext *MemCxt = WasmEdge_CallingFrameGetMemoryInstance(CallFrameCxt, 0);
    if (!MemCxt) return WasmEdge_Result_Fail;

    // Get parameters
    uint32_t PathPtr = WasmEdge_ValueGetI32(In[0]);
    uint32_t PathLen = WasmEdge_ValueGetI32(In[1]);
    uint32_t Threads = WasmEdge_ValueGetI32(In[2]);
    uint32_t GPULayers = WasmEdge_ValueGetI32(In[3]);

    // Get pointer to string data in Wasm memory
    uint8_t *PathData = WasmEdge_MemoryInstanceGetPointer(MemCxt, PathPtr, PathLen);
    if (!PathData) return WasmEdge_Result_Fail;

    // Copy string with null terminator
    std::string ModelPath(reinterpret_cast<char *>(PathData), PathLen);

    // Initialize global context
    if (global_ctx) {
        rwkv_free(global_ctx);
        global_ctx = nullptr;
    }

    global_ctx = rwkv_init_from_file(ModelPath.c_str(), Threads, GPULayers);
    return global_ctx ? WasmEdge_Result_Success : WasmEdge_Result_Fail;
}

// Host function: eval(token: i32) -> i32
// Evaluates a single token using the loaded RWKV model
extern "C" WasmEdge_Result RWKV_Eval(void *, const WasmEdge_CallingFrameContext *CallFrameCxt, const WasmEdge_Value *In, WasmEdge_Value *Out) {
    if (!global_ctx) return WasmEdge_Result_Fail;

    uint32_t Token = WasmEdge_ValueGetI32(In[0]);

    size_t StateLen = rwkv_get_state_len(global_ctx);
    size_t LogitsLen = rwkv_get_logits_len(global_ctx);

    std::vector<float> State(StateLen, 0.0f);
    std::vector<float> Logits(LogitsLen, 0.0f);

    rwkv_init_state(global_ctx, State.data());

    bool Ok = rwkv_eval(global_ctx, Token, nullptr, State.data(), Logits.data());

    Out[0] = WasmEdge_ValueGenI32(Ok ? 1 : 0);
    return WasmEdge_Result_Success;
}

// Create module instance
extern "C" WasmEdge_ModuleInstanceContext *WasmEdgeRWKVModuleCreate() {
    // Create module with name "wasmedge_rwkv"
    WasmEdge_String ModName = WasmEdge_StringCreateByCString("wasmedge_rwkv");
    WasmEdge_ModuleInstanceContext *Mod = WasmEdge_ModuleInstanceCreate(ModName);
    WasmEdge_StringDelete(ModName);
    if (!Mod) return nullptr;

    // Add load_model function
    WasmEdge_String FuncName = WasmEdge_StringCreateByCString("load_model");
    WasmEdge_ValType ParamTypes[4] = {
        WasmEdge_ValTypeGenI32(),
        WasmEdge_ValTypeGenI32(),
        WasmEdge_ValTypeGenI32(),
        WasmEdge_ValTypeGenI32()
    };
    WasmEdge_FunctionTypeContext *FuncType = WasmEdge_FunctionTypeCreate(ParamTypes, 4, nullptr, 0);
    WasmEdge_FunctionInstanceContext *HostFunc = WasmEdge_FunctionInstanceCreate(FuncType, RWKV_LoadModel, nullptr, 0);
    WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, HostFunc);
    WasmEdge_StringDelete(FuncName);
    WasmEdge_FunctionTypeDelete(FuncType);

    // Add eval function
    FuncName = WasmEdge_StringCreateByCString("eval");
    WasmEdge_ValType EvalParamTypes[1] = { WasmEdge_ValTypeGenI32() };
    WasmEdge_ValType EvalReturnTypes[1] = { WasmEdge_ValTypeGenI32() };
    FuncType = WasmEdge_FunctionTypeCreate(EvalParamTypes, 1, EvalReturnTypes, 1);
    HostFunc = WasmEdge_FunctionInstanceCreate(FuncType, RWKV_Eval, nullptr, 0);
    WasmEdge_ModuleInstanceAddFunction(Mod, FuncName, HostFunc);
    WasmEdge_StringDelete(FuncName);
    WasmEdge_FunctionTypeDelete(FuncType);

    return Mod;
}

// Plugin function descriptors
static WasmEdge_FunctionDescriptor RWKVFunctions[] = {
    {"load_model", RWKV_LoadModel, nullptr, 4, 0},
    {"eval", RWKV_Eval, nullptr, 1, 1}
};

// Plugin module descriptor
static WasmEdge_ModuleDescriptor ModuleDesc[] = {
    {
        "wasmedge_rwkv",
        "RWKV Inference Plugin for WasmEdge",
        WasmEdgeRWKVModuleCreate,
        sizeof(RWKVFunctions) / sizeof(RWKVFunctions[0]),
        RWKVFunctions
    }
};

// Plugin descriptor
static const WasmEdge_PluginDescriptor PluginDesc = {
    .Name = "wasmedge_rwkv",
    .Description = "RWKV Inference Plugin for WasmEdge",
    .APIVersion = 2, // Use explicit API version as per WasmEdge plugin API
    .Version = {1, 0, 0, 0},
    .ModuleCount = 1,
    .ModuleDescriptions = ModuleDesc
};

// Plugin entrypoint
extern "C" WASMEDGE_CAPI_EXPORT const WasmEdge_PluginDescriptor *WasmEdge_Plugin_GetDescriptor() {
    return &PluginDesc;
}
