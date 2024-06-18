#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;

Expect<void> Executor::instantiate(Runtime::StoreManager &,
                                   Runtime::Instance::ComponentInstance &,
                                   const AST::Component::StartSection &) {
  spdlog::warn("start section is not supported yet"sv);
  return {};
}

} // namespace Executor
} // namespace WasmEdge
