// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/stackmgr.h - Stack Manager definition ---------------===//
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

#include "common.h"
#include "entry/frame.h"
#include "entry/label.h"
#include "entry/value.h"
#include "support/casting.h"

#include <cassert>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

namespace SSVM {
namespace Executor {

class StackManager {
  struct FrameAndStackSize {
    FrameAndStackSize() = delete;
    template <typename... ArgsT>
    FrameAndStackSize(size_t LabelStackSize, ArgsT &&... Args)
        : LabelStackSize(LabelStackSize), F(std::forward<ArgsT>(Args)...) {}
    size_t LabelStackSize;
    Frame F;
  };

public:
  StackManager() {
    Stack.reserve(2048U);
    FrameStack.reserve(16U);
    LabelStack.reserve(64U);
  };
  ~StackManager() = default;

  size_t size() const { return Stack.size(); }

  /// Getters of top entry of stack.
  Value &getTop() {
    /// Check the size of stack.
    assert(!Stack.empty());
    /// Get pointer.
    return Stack.back();
  }

  /// Getters of top entry of stack.
  ErrCode getBottomN(unsigned int N, Value *&Values) {
    /// Check the size of stack.
    if (Stack.size() < N) {
      return ErrCode::StackEmpty;
    }
    /// Get pointer.
    Values = &Stack[N];
    return ErrCode::Success;
  }

  /// Push a new entry to stack.
  template <typename T> ErrCode push(T &&Val) {
    Stack.push_back(std::forward<T>(Val));
    return ErrCode::Success;
  }

  /// Pop and return the top entry.
  template <typename T> ErrCode pop(T &Entry) {
    /// Check the size of stack.
    if (Stack.empty())
      return ErrCode::StackEmpty;

    /// Move value.
    Entry = std::move(Stack.back());

    /// Drop the top entry.
    Stack.pop_back();

    return ErrCode::Success;
  }

  /// Pop the top entry.
  ErrCode pop() {
    /// Check the size of stack.
    if (Stack.empty())
      return ErrCode::StackEmpty;

    /// Drop the top entry.
    Stack.pop_back();
    return ErrCode::Success;
  }

  ErrCode pushFrame(unsigned int ModuleAddr, unsigned int Arity,
                    unsigned int Coarity) {
    FrameStack.emplace_back(LabelStack.size(), ModuleAddr, Stack.size() - Arity,
                            Coarity);
    return ErrCode::Success;
  }

  ErrCode popFrame() {
    unsigned int LabelPoped;
    return popFrame(LabelPoped);
  }

  ErrCode popFrame(unsigned int &LabelPoped) {
    /// Check the size of stack.
    if (FrameStack.empty())
      return ErrCode::StackEmpty;

    {
      const auto &F = FrameStack.back();

      assert(Stack.size() >= F.F.getCoarity());
      assert(LabelStack.size() >= F.LabelStackSize);
      LabelPoped = LabelStack.size() - F.LabelStackSize;
      LabelStack.erase(LabelStack.begin() + F.LabelStackSize, LabelStack.end());

      /// Keep return values and clean other values
      assert(Stack.size() - F.F.getCoarity() >= F.F.getStackSize());
      Stack.erase(Stack.begin() + F.F.getStackSize(),
                  Stack.end() - F.F.getCoarity());

      assert(LabelStack.empty() ||
             Stack.size() >= LabelStack.back().getStackSize());
    }

    FrameStack.pop_back();
    return ErrCode::Success;
  }

  ErrCode pushLabel(unsigned int Coarity,
                    AST::BlockControlInstruction *Instr = nullptr) {
    LabelStack.emplace_back(Stack.size(), Coarity, Instr);
    return ErrCode::Success;
  }

  ErrCode popLabel() {
    /// Check the size of stack.
    if (LabelStack.empty())
      return ErrCode::StackEmpty;

    {
      const auto &L = LabelStack.back();
      assert(Stack.size() >= L.getCoarity());
      assert(!FrameStack.empty() &&
             FrameStack.back().F.getStackSize() <= L.getStackSize());
      assert(Stack.size() - L.getCoarity() >= L.getStackSize());
      Stack.erase(Stack.begin() + L.getStackSize(),
                  Stack.end() - L.getCoarity());
    }

    LabelStack.pop_back();
    return ErrCode::Success;
  }

  /// Getter of module address.
  unsigned int getModuleAddr() const {
    return FrameStack.back().F.getModuleAddr();
  }

  /// Getter for stack offset of local values by index.
  unsigned int getOffset(unsigned int Idx) const {
    return FrameStack.back().F.getOffset(Idx);
  }

  /// Get the top of count of label.
  ErrCode getLabelWithCount(Label *&L, unsigned int Count) {
    /// Check is there at least count + 1 labels.
    if (LabelStack.size() < Count + 1)
      return ErrCode::WrongInstanceAddress;

    /// Get the (count + 1)-th top of label.
    unsigned int Idx = LabelStack.size() - Count - 1;
    L = &LabelStack[Idx];
    return ErrCode::Success;
  }

  /// Reset stack.
  ErrCode reset() {
    Stack.clear();
    LabelStack.clear();
    FrameStack.clear();
    return ErrCode::Success;
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> Stack;
  std::vector<Label> LabelStack;
  std::vector<FrameAndStackSize> FrameStack;
  /// @}
};

} // namespace Executor
} // namespace SSVM
