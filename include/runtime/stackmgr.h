// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/stackmgr.h - Stack Manager definition ------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Stack Manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "runtime/instance/module.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {

class StackManager {
public:
  using Value = ValVariant;

  struct Frame {
    Frame() = delete;
    Frame(const Instance::ModuleInstance *Mod, AST::InstrView::iterator FromIt,
          uint32_t L, uint32_t A, uint32_t V, uint32_t H) noexcept
        : Module(Mod), From(FromIt), Locals(L), Arity(A), VPos(V), HPos(H) {}
    const Instance::ModuleInstance *Module;
    AST::InstrView::iterator From;
    uint32_t Locals;
    uint32_t Arity;
    uint32_t VPos;
    uint32_t HPos;
  };

  struct Handler {
    Handler(AST::InstrView::iterator TryIt, uint32_t V, uint32_t F,
            Span<const AST::Instruction::CatchDescriptor> C)
        : Try(TryIt), VPos(V), FPos(F), CatchClause(C) {}
    AST::InstrView::iterator Try;
    uint32_t VPos;
    uint32_t FPos;
    Span<const AST::Instruction::CatchDescriptor> CatchClause;
  };

  /// Stack manager provides the stack control for Wasm execution with VALIDATED
  /// modules. All operations of instructions passed validation, therefore no
  /// unexpect operations will occur.
  StackManager() noexcept {
    ValueStack.reserve(2048U);
    FrameStack.reserve(16U);
    HandlerStack.reserve(16U);
  }
  ~StackManager() = default;

  /// Getter of stack size.
  size_t size() const noexcept { return ValueStack.size(); }

  /// Unsafe getter of top entry of stack.
  Value &getTop() { return ValueStack.back(); }

  /// Unsafe getter of top N-th value entry of stack.
  Value &getTopN(uint32_t Offset) noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    return ValueStack[ValueStack.size() - Offset];
  }

  /// Unsafe getter of top N value entries of stack.
  Span<Value> getTopSpan(uint32_t N) {
    return Span<Value>(ValueStack.end() - N, N);
  }

  /// Push a new value entry to stack.
  template <typename T> void push(T &&Val) {
    ValueStack.push_back(std::forward<T>(Val));
  }

  /// Push a vector of value to stack
  void pushValVec(const std::vector<Value> &ValVec) {
    ValueStack.insert(ValueStack.end(), ValVec.begin(), ValVec.end());
  }

  /// Unsafe Pop and return the top entry.
  Value pop() {
    Value V = std::move(ValueStack.back());
    ValueStack.pop_back();
    return V;
  }

  /// Unsafe pop and return the top N entries.
  std::vector<Value> pop(uint32_t N) {
    std::vector<Value> Vec;
    Vec.reserve(N);
    std::move(ValueStack.end() - N, ValueStack.end(), std::back_inserter(Vec));
    ValueStack.erase(ValueStack.end() - N, ValueStack.end());
    return Vec;
  }

  /// Push a new frame entry to stack.
  void pushFrame(const Instance::ModuleInstance *Module,
                 AST::InstrView::iterator From, uint32_t LocalNum = 0,
                 uint32_t Arity = 0, bool IsTailCall = false) noexcept {
    if (likely(!IsTailCall)) {
      FrameStack.emplace_back(Module, From, LocalNum, Arity,
                              static_cast<uint32_t>(ValueStack.size()),
                              static_cast<uint32_t>(HandlerStack.size()));
    } else {
      assuming(!FrameStack.empty());
      assuming(FrameStack.back().VPos >= FrameStack.back().Locals);
      assuming(FrameStack.back().VPos - FrameStack.back().Locals <=
               ValueStack.size() - LocalNum);
      ValueStack.erase(ValueStack.begin() + FrameStack.back().VPos -
                           FrameStack.back().Locals,
                       ValueStack.end() - LocalNum);
      HandlerStack.erase(HandlerStack.begin() + FrameStack.back().HPos,
                         HandlerStack.end());
      FrameStack.back().Module = Module;
      FrameStack.back().Locals = LocalNum;
      FrameStack.back().Arity = Arity;
      FrameStack.back().VPos = static_cast<uint32_t>(ValueStack.size());
      FrameStack.back().HPos = static_cast<uint32_t>(HandlerStack.size());
    }
  }

  /// Unsafe pop top frame.
  AST::InstrView::iterator popFrame() noexcept {
    assuming(!FrameStack.empty());
    assuming(FrameStack.back().VPos >= FrameStack.back().Locals);
    assuming(FrameStack.back().VPos - FrameStack.back().Locals <=
             ValueStack.size() - FrameStack.back().Arity);
    ValueStack.erase(ValueStack.begin() + FrameStack.back().VPos -
                         FrameStack.back().Locals,
                     ValueStack.end() - FrameStack.back().Arity);
    auto From = FrameStack.back().From;
    FrameStack.pop_back();
    return From;
  }

  /// Push handler for try-catch block.
  void
  pushHandler(AST::InstrView::iterator TryIt, uint32_t BlockParamNum,
              Span<const AST::Instruction::CatchDescriptor> Catch) noexcept {
    HandlerStack.emplace_back(
        TryIt, static_cast<uint32_t>(ValueStack.size()) - BlockParamNum,
        static_cast<uint32_t>(FrameStack.size()), Catch);
  }

  /// Erase the stacks until the exception handler is on the top of the stack.
  /// Pop the top handler and associated Values should remain on the top of
  /// ValueStack.
  Handler popTopHandler(uint32_t AssocValSize) noexcept {
    assuming(!HandlerStack.empty());
    assuming(HandlerStack.back().VPos <= ValueStack.size() - AssocValSize);
    ValueStack.erase(ValueStack.begin() + HandlerStack.back().VPos,
                     ValueStack.end() - AssocValSize);
    FrameStack.erase(FrameStack.begin() + HandlerStack.back().FPos,
                     FrameStack.end());
    auto TopHandler = std::move(HandlerStack.back());
    HandlerStack.pop_back();
    return TopHandler;
  }

  /// Check whether handler stack is empty
  bool isHandlerStackEmpty() noexcept { return HandlerStack.empty(); }

  /// Unsafe erase value stack.
  void eraseValueStack(uint32_t EraseBegin, uint32_t EraseEnd) noexcept {
    assuming(EraseEnd <= EraseBegin && EraseBegin <= ValueStack.size());
    ValueStack.erase(ValueStack.end() - EraseBegin,
                     ValueStack.end() - EraseEnd);
  }

  /// Unsafe erase top Num element of exception handler stack
  void eraseHandlerStack(uint32_t Num) noexcept {
    HandlerStack.erase(HandlerStack.end() - Num, HandlerStack.end());
  }

  /// Unsafe leave top label.
  AST::InstrView::iterator
  maybePopFrameOrHandler(AST::InstrView::iterator PC) noexcept {
    if (FrameStack.size() > 1 && PC->isExprLast()) {
      // Noted that there's always a base frame in stack.
      return popFrame();
    }
    if (PC->isTryBlockLast()) {
      HandlerStack.pop_back();
    }
    return PC;
  }

  /// Unsafe getter of module address.
  const Instance::ModuleInstance *getModule() const noexcept {
    assuming(!FrameStack.empty());
    return FrameStack.back().Module;
  }

  /// Reset stack.
  void reset() noexcept {
    ValueStack.clear();
    FrameStack.clear();
    HandlerStack.clear();
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> ValueStack;
  std::vector<Frame> FrameStack;
  std::vector<Handler> HandlerStack;
  /// @}
};

} // namespace Runtime
} // namespace WasmEdge
