//===-- ssvm/executor/worker/provider.h - Instruction Privider Class ------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of instruction provider class for worker.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "executor/common.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Executor {

class InstrProvider {
public:
  /// Enum class of instruction sequence type.
  enum class SeqType : unsigned int { Expression = 0, Block, FunctionCall };

  InstrProvider() = default;
  ~InstrProvider() = default;

  /// Get the next instruction.
  AST::Instruction *getNextInstr();

  /// Get sequence type of top scope.
  SeqType getTopScopeType();

  /// Get sequences stack size.
  unsigned int getScopeSize() { return Iters.size(); }

  /// Push instruction sequence.
  ErrCode pushInstrs(SeqType Type, const AST::InstrVec &Instrs);

  /// Pop instruction sequence.
  ErrCode popInstrs();

  /// Reset instruction provider.
  ErrCode reset();

private:
  /// Stack of instruction sequences.
  struct InstrScope {
    InstrScope(SeqType Type, AST::InstrIter Curr, AST::InstrIter End)
        : Type(Type), Curr(Curr), End(End) {}
    SeqType Type;
    AST::InstrIter Curr;
    AST::InstrIter End;
  };
  std::vector<InstrScope> Iters;
};

} // namespace Executor
} // namespace SSVM
