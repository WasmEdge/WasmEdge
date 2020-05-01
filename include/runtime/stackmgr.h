// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/runtime/stackmgr.h - Stack Manager definition ----------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Stack Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/instruction.h"
#include "common/value.h"
#include "support/casting.h"
#include "support/span.h"

#include <memory>
#include <vector>

namespace SSVM {
namespace Runtime {

class StackManager {
public:
  struct Label {
    Label() = delete;
    Label(const uint32_t S, const uint32_t C,
          const AST::BlockControlInstruction *Instr)
        : StackSize(S), Coarity(C), Target(Instr) {}
    uint32_t StackSize;
    uint32_t Coarity;
    const AST::BlockControlInstruction *Target;
  };

  struct Frame {
    Frame() = delete;
    Frame(const uint32_t Addr, const uint32_t VS, const uint32_t LS,
          const uint32_t C)
        : ModAddr(Addr), VStackSize(VS), LStackSize(LS), Coarity(C) {}
    uint32_t ModAddr;
    uint32_t VStackSize;
    uint32_t LStackSize;
    uint32_t Coarity;
  };

  using Value = ValVariant;

  /// Stack manager provides the stack control for Wasm execution with VALIDATED
  /// modules. All operations of instructions passed validation, therefore no
  /// unexpect operations will occur.
  StackManager() {
    ValueStack.reserve(2048U);
    LabelStack.reserve(64U);
    FrameStack.reserve(16U);
  };
  ~StackManager() = default;

  /// Getter of stack size.
  size_t size() const { return ValueStack.size(); }

  /// Unsafe Getter of top entry of stack.
  Value &getTop() { return ValueStack.back(); }

  /// Unsafe Getter of bottom N-th value entry of stack.
  Value &getBottomN(uint32_t N) { return ValueStack[N]; }

  /// Unsafe Getter of top N value entries of stack.
  Expect<Span<Value>> getTopSpan(uint32_t N) {
    if (unlikely(ValueStack.size() < N)) {
      return Unexpect(ErrCode::StackEmpty);
    }
    return Span<Value>(ValueStack.end() - N, N);
  }

  /// Push a new value entry to stack.
  template <typename T> void push(T &&Val) {
    ValueStack.push_back(std::forward<T>(Val));
  }

  /// Unsafe Pop and return the top entry.
  Value pop() {
    Value V = std::move(ValueStack.back());
    ValueStack.pop_back();
    return V;
  }

  /// Push a new frame entry to stack.
  void pushFrame(const uint32_t ModuleAddr, const uint32_t Arity,
                 const uint32_t Coarity) {
    FrameStack.emplace_back(ModuleAddr, ValueStack.size() - Arity,
                            LabelStack.size(), Coarity);
  }

  /// Unsafe pop top frame. Return number of popped label.
  uint32_t popFrame() {
    uint32_t LabelPopped = LabelStack.size() - FrameStack.back().LStackSize;
    LabelStack.erase(LabelStack.begin() + FrameStack.back().LStackSize,
                     LabelStack.end());
    ValueStack.erase(ValueStack.begin() + FrameStack.back().VStackSize,
                     ValueStack.end() - FrameStack.back().Coarity);
    FrameStack.pop_back();
    return LabelPopped;
  }

  /// Push a new label entry to stack.
  void pushLabel(const uint32_t Coarity,
                 const AST::BlockControlInstruction *Instr = nullptr) {
    LabelStack.emplace_back(ValueStack.size(), Coarity, Instr);
  }

  /// Unsafe pop top label.
  void popLabel(const uint32_t Cnt = 1) {
    const auto &L = getLabelWithCount(Cnt - 1);
    ValueStack.erase(ValueStack.begin() + L.StackSize,
                     ValueStack.end() - L.Coarity);
    for (uint32_t I = 0; I < Cnt; ++I) {
      LabelStack.pop_back();
    }
  }

  /// Unsafe getter of module address.
  uint32_t getModuleAddr() const { return FrameStack.back().ModAddr; }

  /// Unsafe getter for stack offset of local values by index.
  uint32_t getOffset(uint32_t Idx) const {
    return FrameStack.back().VStackSize + Idx;
  }

  /// Unsafe getter of the top count of label which index start from 0.
  const Label &getLabelWithCount(const uint32_t Count) const {
    return LabelStack[LabelStack.size() - Count - 1];
  }

  /// Reset stack.
  void reset() {
    ValueStack.clear();
    LabelStack.clear();
    FrameStack.clear();
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> ValueStack;
  std::vector<Label> LabelStack;
  std::vector<Frame> FrameStack;
  /// @}
};

} // namespace Runtime
} // namespace SSVM
