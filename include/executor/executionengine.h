#pragma once

#include "ast/module.h"
#include <memory>

namespace SSVM {
namespace ExecutionEngine {

enum class ErrCode : unsigned int {
  Success = 0,
  Invalid
};

class ExecutionEngine {
public:
  ExecutionEngine() = default;
  ~ExecutionEngine() = default;

  /// Retrieve ownership of wasm Module.
  ErrCode setModule(std::unique_ptr<AST::Module> &Module);

private:
  std::unique_ptr<AST::Module> Mod;

};

} // namespace ExecutionEngine
} // namespace SSVM
