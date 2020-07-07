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
    Label(const uint32_t S, const uint32_t A,
          const AST::BlockControlInstruction *Instr)
        : VStackOff(S), Arity(A), Target(Instr) {}
    uint32_t VStackOff;
    uint32_t Arity;
    const AST::BlockControlInstruction *Target;
  };

  struct Frame {
    Frame() = delete;
    Frame(const uint32_t Addr, const uint32_t VS, const uint32_t LS,
          const uint32_t A, const bool Dummy = false)
        : ModAddr(Addr), VStackOff(VS), LStackOff(LS), Arity(A),
          IsDummy(Dummy) {}
    uint32_t ModAddr;
    uint32_t VStackOff;
    uint32_t LStackOff;
    uint32_t Arity;
    bool IsDummy;
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
  Span<Value> getTopSpan(uint32_t N) {
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
  void pushFrame(const uint32_t ModuleAddr, const uint32_t LocalNum = 0,
                 const uint32_t ArityNum = 0) {
    FrameStack.emplace_back(ModuleAddr, ValueStack.size() - LocalNum,
                            LabelStack.size(), ArityNum);
  }

  /// Push a dummy frame for invokation base.
  void pushDummyFrame() {
    FrameStack.emplace_back(0, ValueStack.size(), LabelStack.size(), 0, true);
  }

  /// Unsafe pop top frame. Return number of popped label.
  uint32_t popFrame() {
    uint32_t LabelPopped = LabelStack.size() - FrameStack.back().LStackOff;
    LabelStack.erase(LabelStack.begin() + FrameStack.back().LStackOff,
                     LabelStack.end());
    ValueStack.erase(ValueStack.begin() + FrameStack.back().VStackOff,
                     ValueStack.end() - FrameStack.back().Arity);
    FrameStack.pop_back();
    return LabelPopped;
  }

  /// Push a new label entry to stack.
  void pushLabel(const uint32_t LocalNum = 0, const uint32_t ArityNum = 0,
                 const AST::BlockControlInstruction *Instr = nullptr) {
    LabelStack.emplace_back(ValueStack.size() - LocalNum, ArityNum, Instr);
  }

  /// Unsafe pop top label.
  void popLabel(const uint32_t Cnt = 1) {
    const auto &L = getLabelWithCount(Cnt - 1);
    ValueStack.erase(ValueStack.begin() + L.VStackOff,
                     ValueStack.end() - L.Arity);
    for (uint32_t I = 0; I < Cnt; ++I) {
      LabelStack.pop_back();
    }
  }

  /// Unsafe leave top label.
  void leaveLabel() { LabelStack.pop_back(); }

  /// Unsafe getter of module address.
  uint32_t getModuleAddr() const { return FrameStack.back().ModAddr; }

  /// Unsafe getter for stack offset of local values by index.
  uint32_t getOffset(uint32_t Idx) const {
    return FrameStack.back().VStackOff + Idx;
  }

  /// Unsafe getter of the top count of label which index start from 0.
  const Label &getLabelWithCount(const uint32_t Count) const {
    return LabelStack[LabelStack.size() - Count - 1];
  }

  /// Unsafe checker of top frame is a dummy frame.
  bool isTopDummyFrame() { return FrameStack.back().IsDummy; }

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
