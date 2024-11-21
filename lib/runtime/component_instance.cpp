#include "runtime/instance/component.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

using namespace AST::Component;

std::string_view ComponentInstance::getComponentName() const noexcept {
  return CompName;
}

void ComponentInstance::addModule(const AST::Module &M) noexcept {
  ModList.emplace_back(M);
}
const AST::Module &ComponentInstance::getModule(uint32_t Index) const noexcept {
  return ModList[Index];
}

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge