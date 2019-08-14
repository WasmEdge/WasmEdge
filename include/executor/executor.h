#pragma once

#include "ast/module.h"
#include "common.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "worker.h"
#include <memory>

namespace SSVM {
namespace Executor {

class Executor {
public:
  Executor() = default;
  ~Executor() = default;

  /// Retrieve ownership of Wasm Module.
  ErrCode setModule(std::unique_ptr<AST::Module> &Module);
  /// Instantiate Wasm Module.
  ErrCode instantiate();
  /// Execute Wasm.
  ErrCode run();
  /// Create a new executor with Stack and Store.
  Executor createExecutor();

private:
  std::unique_ptr<AST::Module> Mod = nullptr;
  StackManager StackMgr;
  StoreManager StoreMgr;
};

} // namespace Executor
} // namespace SSVM
