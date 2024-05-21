#include "sd_module.h"
#include "sd_func.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

SDModule::SDModule() : ModuleInstance("stable_diffusion") {
  addHostFunc("create_context", std::make_unique<SDCreateContext>(Env));
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge