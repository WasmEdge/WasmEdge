#include "sb_module.h"
#include "sb_func.h"

namespace WasmEdge {
namespace Host {

SBModule::SBModule() : ModuleInstance("stable_diffusion") {
  addHostFunc("create_context", std::make_unique<SBCreateContext>(Env));
}

} // namespace Host
} // namespace WasmEdge