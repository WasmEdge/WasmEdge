#include <wasmedge/wasmedge.h>
#include "rwkv.h"

#include <memory>
#include <vector>
#include <string>

static struct rwkv_context* global_ctx = nullptr;

// RWKV.load_model(path: string, threads: i32, gpu_layers: i32) -> void
extern "C" WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_Plugin_LoadModel(WasmEdge_ModuleInstanceContext *, WasmEdge_MemoryInstanceContext *mem, WasmEdge_Value *params, WasmEdge_Value *rets) {
  uint32_t offset = WasmEdge_ValueGetI32(params[0]);
  uint32_t length = WasmEdge_ValueGetI32(params[1]);
  uint32_t threads = WasmEdge_ValueGetI32(params[2]);
  uint32_t gpu_layers = WasmEdge_ValueGetI32(params[3]);

  // Use reinterpret_cast for uint8_t* to char* conversion
  char *model_path = reinterpret_cast<char *>(WasmEdge_MemoryInstanceGetPointer(mem, offset, length));
  if (!model_path) return WasmEdge_Result_Fail;

  global_ctx = rwkv_init_from_file(model_path, threads, gpu_layers);
  return global_ctx ? WasmEdge_Result_Success : WasmEdge_Result_Fail;
}

// RWKV.eval(token: i32) -> i32
extern "C" WASMEDGE_CAPI_EXPORT WasmEdge_Result WasmEdge_Plugin_Eval(WasmEdge_ModuleInstanceContext *, WasmEdge_MemoryInstanceContext *, WasmEdge_Value *params, WasmEdge_Value *rets) {
  if (!global_ctx) return WasmEdge_Result_Fail;

  uint32_t token = WasmEdge_ValueGetI32(params[0]);

  size_t state_len = rwkv_get_state_len(global_ctx);
  size_t logits_len = rwkv_get_logits_len(global_ctx);

  std::vector<float> state(state_len, 0.0f);
  std::vector<float> logits(logits_len, 0.0f);

  rwkv_init_state(global_ctx, state.data());

  bool ok = rwkv_eval(global_ctx, token, nullptr, state.data(), logits.data());

  WasmEdge_Value result = WasmEdge_ValueGenI32(ok ? 1 : 0);
  rets[0] = result;
  return WasmEdge_Result_Success;
}

// Define plugin descriptor
static const WasmEdge_PluginDescriptor PluginDesc = {
  .Name = "wasmedge_rwkv",
  .Description = "RWKV Inference Plugin for WasmEdge",
  .APIVersion = 2, // Fallback to version 2; check wasmedge.h for exact value
  .Version = {1, 0, 0, 0},
  .ModuleCount = 0,
  .ModuleDescriptions = nullptr,
  .Create = nullptr,
  .Destroy = nullptr
};

// Export plugin descriptor
extern "C" WASMEDGE_CAPI_EXPORT const WasmEdge_PluginDescriptor* WasmEdge_Plugin_GetDescriptor(void) {
  return &PluginDesc;
}
