// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/interpreter/engine/provider.h - Instruction Privider Class ---===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of instruction provider class for
/// interpreter.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/instruction.h"
#include "common/errcode.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Interpreter {

class InstrProvider {
public:
  /// Enum class of instruction sequence type.
  enum class SeqType : uint8_t { Expression = 0, Block, FunctionCall };

  InstrProvider() = default;
  ~InstrProvider() = default;

  /// Get the next instruction.
  const AST::Instruction *getNextInstr();

  /// Get sequence type of top scope.
  SeqType getTopScopeType() const {
    return (Iters.size() > 0) ? Iters.back().Type : SeqType::Expression;
  }

  /// Get sequences stack size.
  uint32_t getScopeSize() const { return Iters.size(); }

  /// Push instruction sequence.
  void pushInstrs(SeqType Type) {
    Iters.emplace_back(Type, EmptyVec.cbegin(), EmptyVec.cend());
  }
  void pushInstrs(SeqType Type, const AST::InstrVec &Instrs) {
    Iters.emplace_back(Type, Instrs.cbegin(), Instrs.cend());
  }

  /// Unsafe pop instruction sequence. Should be correct according to
  /// validation.
  void popInstrs() { Iters.pop_back(); }

  /// Reset instruction provider.
  void reset() { Iters.clear(); }

private:
  /// Stack of instruction sequences.
  struct InstrScope {
    InstrScope(const SeqType Type, AST::InstrIter Curr, AST::InstrIter End)
        : Type(Type), Curr(Curr), End(End) {}
    SeqType Type;
    AST::InstrIter Curr;
    AST::InstrIter End;
  };
  AST::InstrVec EmptyVec;
  std::vector<InstrScope> Iters;
};

} // namespace Interpreter
} // namespace SSVM
