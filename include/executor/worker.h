#pragma once

#include "ast/instruction.h"
#include "common.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "support/casting.h"

#include <cstdint>

namespace SSVM {
namespace Executor {

class Worker {

public:
  using Byte = uint8_t;
  using Bytes =  std::vector<Byte>;
  using Instructions = std::vector<AST::Instruction*>;

public:
  /// Worker are not allowed to create without Store and Stack.
  Worker() = delete;
  explicit Worker(StoreManager &Store, StackManager &Stack)
    : StoreMgr(Store), StackMgr(Stack) {
  }

  ~Worker() = default;

  /// Prepare input data for calldatacopy
  ErrCode setArguments(Bytes &Input);
  /// Prepare Wasm bytecode for execution
  ErrCode setCode(std::vector<std::unique_ptr<AST::Instruction>> &Instrs);
  /// Execution Wasm bytecode with given input data.
  ErrCode run();

private:
  /// Arguments
  Bytes Args;
  /// Instructions of execution code.
  Instructions Instrs;
  /// Reference to Executor's Store
  StoreManager &StoreMgr;
  /// Reference to Executor's Stack
  StackManager &StackMgr;
};

} // namespace Executor
} // namespace SSVM
