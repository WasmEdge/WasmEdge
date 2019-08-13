#pragma once

#include "ast/instruction.h"
#include "common.h"
#include "stackmgr.h"
#include "storemgr.h"

#include <cstdint>

namespace SSVM {
namespace ExecutionEngine {

class Executor {
public:
  using Byte = uint8_t;
  using Instructions = std::vector<AST::Instruction*>;
  enum class ErrCode : unsigned int {
    Success = 0,
    Invalid
  };

public:
  Executor() = default;
  ~Executor() = default;

  ErrCode setArguments(std::vector<Byte> &Input) {
    Args.assign(Input.begin(), Input.end());
    return ErrCode::Success;
  }

  ErrCode setCode(std::vector<std::unique_ptr<AST::Instruction>> &Instrs) {
    for (auto &Instr : Instrs) {
      this->Instrs.push_back(Instr.get());
    }
    return ErrCode::Success;
  }

  ErrCode run() { return ErrCode::Success; }

private:
  /// Executor State
  enum class State : unsigned int {
    Inited,
    Terminated,
    Finished
  };
  /// Arguments
  std::vector<Byte> Args;
  /// Instructions of execution code.
  Instructions Instrs;
};

} // namespace ExecutorEngine
} // namespace SSVM
