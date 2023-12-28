#include "wasi_random/module.h"
#include "wasi_random/func.h"
#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals;

WasiRandomModule::WasiRandomModule() : ModuleInstance("wasi:random/random"sv) {
  addHostFunc("get_random_bytes"sv, std::make_unique<WasiGetRandomBytes>(Env));
  addHostFunc("get_random_u64"sv, std::make_unique<WasiGetRandomU64>(Env));
}

WasiRandomInsecureModule::WasiRandomInsecureModule()
    : ModuleInstance("wasi:random/insecure"sv) {
  addHostFunc("get_insecure_random_bytes"sv,
              std::make_unique<WasiGetInsecureRandomBytes>(Env));
  addHostFunc("get_insecure_random_u64"sv,
              std::make_unique<WasiGetInsecureRandomU64>(Env));
}

WasiRandomInsecureSeedModule::WasiRandomInsecureSeedModule()
    : ModuleInstance("wasi:random/insecure-seed"sv) {
  addHostFunc("insecure_seed"sv, std::make_unique<WasiInsecureSeed>(Env));
}

} // namespace Host
} // namespace WasmEdge
