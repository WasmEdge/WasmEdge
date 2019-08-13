#pragma once

#include "ast/module.h"
#include "executor.h"
#include "stackmgr.h"
#include "storemgr.h"
#include <memory>

namespace SSVM {
namespace ExecutionEngine {

class ExecutionEngine {
public:
  enum class ErrCode : unsigned int {
    Success = 0,
    Invalid
  };

  ExecutionEngine() = default;
  ~ExecutionEngine() = default;

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

} // namespace ExecutionEngine
} // namespace SSVM
