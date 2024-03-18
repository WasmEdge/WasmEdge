#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "executor/executor.h"

#include "runtime/instance/module.h"

#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace AST::Component;

Expect<void>
Executor::instantiate(Runtime::StoreManager &,
                      Runtime::Instance::ComponentInstance &CompInst,
                      const AST::Component::CanonSection &CanonSec) {
  for (auto &C : CanonSec.getContent()) {
    if (std::holds_alternative<Lift>(C)) {
      // lift wrap a core wasm function to a component function, with proper
      // modification about canonical ABI.

      auto L = std::get<Lift>(C);
      // TODO: apply options
      // L.getOptions();

      auto *FuncInst = CompInst.getCoreFunctionInstance(L.getCoreFuncIndex());
      CompInst.addFunctionInstance(FuncInst);
    } else if (std::holds_alternative<Lower>(C)) {
      // lower sends a component function to a core wasm function, with proper
      // modification about canonical ABI.
      auto L = std::get<Lower>(C);

      auto *FuncInst = CompInst.getFunctionInstance(L.getFuncIndex());

      auto &Opts = L.getOptions();
      for (auto &Opt : Opts) {
        if (std::holds_alternative<StringEncoding>(Opt)) {
          auto Enc = std::get<StringEncoding>(Opt);
          switch (Enc) {
          case StringEncoding::UTF8:
            break;
          case StringEncoding::UTF16:
            break;
          case StringEncoding::Latin1:
            break;
          default:
            break;
          }
          spdlog::warn("incomplete canonical option `string-encoding`");
        } else if (std::holds_alternative<Memory>(Opt)) {
          auto Mem = std::get<Memory>(Opt);
          auto MemIdx = Mem.getMemIndex();
          auto *MemInst = CompInst.getCoreMemoryInstance(MemIdx);
          FuncInst->getModule();
          // TODO: let FuncInst use MemInst here
        } else if (std::holds_alternative<Realloc>(Opt)) {
          spdlog::warn("incomplete canonical option `realloc`");
        } else if (std::holds_alternative<PostReturn>(Opt)) {
          spdlog::warn("incomplete canonical option `post-return`");
        }
      }

      CompInst.addCoreFunctionInstance(FuncInst);
    } else if (std::holds_alternative<ResourceNew>(C)) {
      spdlog::warn("resource is not supported yet");
    } else if (std::holds_alternative<ResourceDrop>(C)) {
      spdlog::warn("resource is not supported yet");
    } else if (std::holds_alternative<ResourceRep>(C)) {
      spdlog::warn("resource is not supported yet");
    }
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
