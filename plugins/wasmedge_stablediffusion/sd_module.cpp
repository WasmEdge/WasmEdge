#include "sd_module.h"
#include "sd_func.h"

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

SDModule::SDModule() : ModuleInstance("stable_diffusion") {
  addHostFunc("create_context", std::make_unique<SDCreateContext>(Env));
  addHostFunc("text_to_image", std::make_unique<SDTextToImage>(Env));
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge