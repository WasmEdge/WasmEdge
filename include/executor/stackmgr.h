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
  struct LabelAndStackSize {
    LabelAndStackSize() = delete;
    template <typename... ArgsT>
    LabelAndStackSize(size_t StackSize, ArgsT &&... Args)
        : StackSize(StackSize),
          L(std::forward<ArgsT>(Args)...) {}
    size_t StackSize;
    Label L;
  };

  struct FrameAndStackSize {
    FrameAndStackSize() = delete;
    template <typename... ArgsT>
    FrameAndStackSize(size_t StackSize, size_t LabelStackSize, ArgsT &&... Args)
        : StackSize(StackSize), LabelStackSize(LabelStackSize),
          F(std::forward<ArgsT>(Args)...) {}
    size_t StackSize;
    size_t LabelStackSize;
    Frame F;
  };

public:
  StackManager() = default;
  ~StackManager() = default;

  /// Getters of top entry of stack.
  Value &getTop() {
    /// Check the size of stack.
    assert(!Stack.empty());
    /// Get pointer.
    return Stack.back();
  }

  /// Getters of top entry of stack.
  ErrCode getTop(Value *&Entry) {
    /// Check the size of stack.
    if (Stack.empty())
      return ErrCode::StackEmpty;
    /// Check is the top entry type matched and get pointer.
    Entry = &Stack.back();
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

  template <typename... ArgsT> ErrCode pushFrame(ArgsT &&... Args) {
    FrameStack.emplace_back(Stack.size(), LabelStack.size(),
                            std::forward<ArgsT>(Args)...);
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

    assert(LabelStack.size() >= FrameStack.back().LabelStackSize);
    LabelPoped = LabelStack.size() - FrameStack.back().LabelStackSize;
    if (LabelStack.size() > FrameStack.back().LabelStackSize) {
      LabelStack.erase(LabelStack.begin() + FrameStack.back().LabelStackSize,
                       LabelStack.end());
    }

    assert(Stack.size() >= FrameStack.back().StackSize);
    if (Stack.size() > FrameStack.back().StackSize) {
      Stack.erase(Stack.begin() + FrameStack.back().StackSize, Stack.end());
    }

    assert(LabelStack.empty() || Stack.size() >= LabelStack.back().StackSize);

    FrameStack.pop_back();
    return ErrCode::Success;
  }

  template <typename... ArgsT> ErrCode pushLabel(ArgsT &&... Args) {
    LabelStack.emplace_back(Stack.size(), std::forward<ArgsT>(Args)...);
    return ErrCode::Success;
  }

  ErrCode popLabel() {
    /// Check the size of stack.
    if (LabelStack.empty())
      return ErrCode::StackEmpty;

    assert(!FrameStack.empty() &&
           FrameStack.back().StackSize <= LabelStack.back().StackSize);
    assert(Stack.size() >= LabelStack.back().StackSize);
    if (Stack.size() > LabelStack.back().StackSize) {
      Stack.erase(Stack.begin() + LabelStack.back().StackSize, Stack.end());
    }

    LabelStack.pop_back();
    return ErrCode::Success;
  }

  /// Get the current toppest frame.
  ErrCode getCurrentFrame(Frame *&F) {
    /// Check is there current frame.
    if (FrameStack.empty())
      return ErrCode::WrongInstanceAddress;

    /// Get the current frame pointer.
    F = &FrameStack.back().F;
    return ErrCode::Success;
  }

  /// Get the top of count of label.
  ErrCode getLabelWithCount(Label *&L, unsigned int Count) {
    /// Check is there at least count + 1 labels.
    if (LabelStack.size() < Count + 1)
      return ErrCode::WrongInstanceAddress;

    /// Get the (count + 1)-th top of label.
    unsigned int Idx = LabelStack.size() - Count - 1;
    L = &LabelStack[Idx].L;
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
  std::vector<LabelAndStackSize> LabelStack;
  std::vector<FrameAndStackSize> FrameStack;
  /// @}
};

} // namespace Executor
} // namespace SSVM
