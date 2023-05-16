#include "wasi_logging/module.h"
#include "wasi_logging/func.h"
#include <string_view>

namespace WasmEdge {
namespace Host {

using namespace std::literals;

WasiLoggingModule::WasiLoggingModule() : ModuleInstance("logging"sv) {
  addHostFunc("log"sv, std::make_unique<WasiLoggingLog>(Env));
}

} // namespace Host
} // namespace WasmEdge