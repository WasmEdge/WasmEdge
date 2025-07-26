#include "wasmedge/Plugin.h"
#include "rwkv.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

static struct rwkv_context* global_ctx = nullptr;

// RWKV.load_model(path: string, threads: i32, gpu_layers: i32) -> void
WASMEDGE_PLUGIN_EXPORT WasiEdgeResult LoadModel(WasmEdge_ModuleInstanceContext *, WasmEdge_MemoryInstanceContext *mem, WasmEdge_Value *params, WasmEdge_Value *rets) {
  uint32_t offset = WasmEdge_ValueGetI32(params[0]);
  uint32_t length = WasmEdge_ValueGetI32(params[1]);
  uint32_t threads = WasmEdge_ValueGetI32(params[2]);
  uint32_t gpu_layers = WasmEdge_ValueGetI32(params[3]);

  char *model_path = static_cast<char *>(WasmEdge_MemoryInstanceGetPointer(mem, offset, length));
  if (!model_path) return WasiEdge_Result_Fail;

  global_ctx = rwkv_init_from_file(model_path, threads, gpu_layers);
  return global_ctx ? WasiEdge_Result_Success : WasiEdge_Result_Fail;
}

// RWKV.eval(token: i32) -> i32
WASMEDGE_PLUGIN_EXPORT WasiEdgeResult Eval(WasmEdge_ModuleInstanceContext *, WasmEdge_MemoryInstanceContext *, WasmEdge_Value *params, WasmEdge_Value *rets) {
  if (!global_ctx) return WasiEdge_Result_Fail;

  uint32_t token = WasmEdge_ValueGetI32(params[0]);

  size_t state_len = rwkv_get_state_len(global_ctx);
  size_t logits_len = rwkv_get_logits_len(global_ctx);

  std::vector<float> state(state_len, 0.0f);
  std::vector<float> logits(logits_len, 0.0f);

  rwkv_init_state(global_ctx, state.data());

  bool ok = rwkv_eval(global_ctx, token, nullptr, state.data(), logits.data());

  WasmEdge_Value result = WasmEdge_ValueGenI32(ok ? 1 : 0);
  rets[0] = result;
  return WasiEdge_Result_Success;
}

// Register plugin functions
WASMEDGE_PLUGIN_DESCRIPTOR_EXPORT
WasmEdge_PluginDescriptor PluginDescriptor = {
    .Name = "rwkv",
    .Description = "RWKV Plugin",
    .APIVersion = 1,
    .Create = nullptr,
    .Destroy = nullptr,
    .ModuleDescriptors = [](uint32_t &Count) -> const WasmEdge_ModuleDescriptor * {
        static WasmEdge_FunctionDescriptor Funcs[] = {
            {"load_model", WasmEdge_ValType_I32, WasmEdge_ValType_Void, LoadModel},
            {"eval", WasmEdge_ValType_I32, WasmEdge_ValType_I32, Eval}
        };
        static WasmEdge_ModuleDescriptor Module = {
            .Name = "rwkv",
            .FuncCount = 2,
            .FuncList = Funcs
        };
        Count = 1;
        return &Module;
    }
};
