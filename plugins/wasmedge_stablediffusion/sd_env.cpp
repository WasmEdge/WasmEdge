
#include "sd_env.h"
namespace WasmEdge::Host::StableDiffusion {
uint32_t SDEnviornment::addContext(sd_ctx_t *sd_ctx) noexcept {
  Contexts.push_back(sd_ctx);
  return Contexts.size() - 1;
}
sd_ctx_t *SDEnviornment::getContext(const uint32_t Id) noexcept {
  return Contexts[Id];
}
} // namespace WasmEdge::Host::StableDiffusion