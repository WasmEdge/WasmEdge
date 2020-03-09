#include "ssvmaddon.h"
#include <napi.h>

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return SSVMAddon::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)
