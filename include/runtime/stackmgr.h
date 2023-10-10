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
          uint32_t L, uint32_t A, uint32_t V, uint32_t H, uint32_t C) noexcept
        : Module(Mod), From(FromIt), Locals(L), Arity(A), VPos(V), HPos(H),
          CPos(C) {}
    const Instance::ModuleInstance *Module;
    AST::InstrView::iterator From;
    uint32_t Locals;
    uint32_t Arity;
    uint32_t VPos;
    uint32_t HPos;
    uint32_t CPos;
  };

  struct Handler {
    Handler(AST::InstrView::iterator E, uint32_t V, uint32_t F, uint32_t H,
            uint32_t C)
        : CatchCaluse(), EndIt(E), VPos(V), FPos(F), HPos(H), CPos(C) {}
    // nullptr stands for catch all clause
    std::vector<
        std::pair<Runtime::Instance::TagInstance *, AST::InstrView::iterator>>
        CatchCaluse;
    AST::InstrView::iterator EndIt;
    uint32_t VPos;
    uint32_t FPos;
    uint32_t HPos;
    uint32_t CPos;
  };

  struct Exception {
    Exception(Runtime::Instance::TagInstance *T, Span<Value> &V,
              AST::InstrView::iterator E)
        : TagInst(T), Val(V.begin(), V.end()), EndIt(E) {}
    Runtime::Instance::TagInstance *TagInst;
    std::vector<Value> Val;
    AST::InstrView::iterator EndIt;
  };

  /// Stack manager provides the stack control for Wasm execution with VALIDATED
  /// modules. All operations of instructions passed validation, therefore no
  /// unexpect operations will occur.
  StackManager() noexcept {
    ValueStack.reserve(2048U);
    FrameStack.reserve(16U);
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
                              static_cast<uint32_t>(HandlerStack.size()),
                              static_cast<uint32_t>(CaughtStack.size()));
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
      CaughtStack.erase(CaughtStack.begin() + FrameStack.back().HPos,
                        CaughtStack.end());
      FrameStack.back().Module = Module;
      FrameStack.back().Locals = LocalNum;
      FrameStack.back().Arity = Arity;
      FrameStack.back().VPos = static_cast<uint32_t>(ValueStack.size());
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

  // Push handler for try-delegate block
  void pushHandler(AST::InstrView::iterator EndIt, uint32_t VSize,
                   uint32_t HOffset, uint32_t COffset) noexcept {
    // The POS is the stack size after jumping to the label
    HandlerStack.emplace_back(
        EndIt, VSize, static_cast<uint32_t>(FrameStack.size()),
        static_cast<uint32_t>(HandlerStack.size()) - HOffset,
        static_cast<uint32_t>(CaughtStack.size()) - COffset);
  }

  // Push handler for try-catch block
  void pushHandler(AST::InstrView::iterator EndIt,
                   uint32_t BlockParamNum) noexcept {
    HandlerStack.emplace_back(
        EndIt, static_cast<uint32_t>(ValueStack.size()) - BlockParamNum,
        static_cast<uint32_t>(FrameStack.size()),
        static_cast<uint32_t>(HandlerStack.size()),
        static_cast<uint32_t>(CaughtStack.size()));
  }

  // Push a handler to the current try block
  void addCatchCaluseToLastHandler(Runtime::Instance::TagInstance *T,
                                   AST::InstrView::iterator It) noexcept {
    HandlerStack.back().CatchCaluse.emplace_back(T, It);
  }

  // Erase the stacks so that the exception handler is on the top of the stack
  // Associated Value should remain on top of ValueStack.
  Handler popToTopHandler(uint32_t AssocValSize) noexcept {
    auto TopHandler = std::move(HandlerStack.back());
    HandlerStack.pop_back();
    assuming(ValueStack.size() - AssocValSize >= TopHandler.VPos);
    ValueStack.erase(ValueStack.begin() + TopHandler.VPos,
                     ValueStack.end() - AssocValSize);
    FrameStack.erase(FrameStack.begin() + TopHandler.FPos, FrameStack.end());
    HandlerStack.erase(HandlerStack.begin() + TopHandler.HPos,
                       HandlerStack.end());
    CaughtStack.erase(CaughtStack.begin() + TopHandler.CPos, CaughtStack.end());
    return TopHandler;
  }

  // Unsafe pop top exception handler
  AST::InstrView::iterator popHandler() noexcept {
    auto EndIt = HandlerStack.back().EndIt;
    HandlerStack.pop_back();
    return EndIt;
  }

  // Check whether handler stack is empty
  bool isHandlerStackEmpty() noexcept { return HandlerStack.empty(); }

  // Push a caught exception to the stack
  void pushCaught(Runtime::Instance::TagInstance *T, Span<Value> V,
                  AST::InstrView::iterator E) noexcept {
    CaughtStack.emplace_back(T, V, E);
  }

  // Unsafe pop top caught exception
  AST::InstrView::iterator popCaught() noexcept {
    auto EndIt = CaughtStack.back().EndIt;
    CaughtStack.pop_back();
    return EndIt;
  }

  /// Unsafe Getter of top N-th value entry of stack.
  Exception &getCaughtTopN(uint32_t Offset) noexcept {
    assuming(0 < Offset && Offset <= CaughtStack.size());
    return *(CaughtStack.end() - Offset);
  }

  /// Unsafe erase value stack.
  void eraseValueStack(uint32_t EraseBegin, uint32_t EraseEnd) noexcept {
    assuming(EraseEnd <= EraseBegin && EraseBegin <= ValueStack.size());
    ValueStack.erase(ValueStack.end() - EraseBegin,
                     ValueStack.end() - EraseEnd);
  }

  // Unsafe erase top Num element of exception handler stack
  void eraseHandlerStack(uint32_t Num) noexcept {
    HandlerStack.erase(HandlerStack.end() - Num, HandlerStack.end());
  }

  // Unsafe erase top Num element of caught exception stack
  void eraseCaughtStack(uint32_t Num) noexcept {
    CaughtStack.erase(CaughtStack.end() - Num, CaughtStack.end());
  }

  /// Unsafe leave top label.
  AST::InstrView::iterator
  maybePopFrameOrHandlerOrCaught(AST::InstrView::iterator PC) noexcept {
    if (FrameStack.size() > 1 && PC->isLast()) {
      // Noted that there's always a base frame in stack.
      return popFrame();
    }
    if (PC->isTryLast()) {
      return popHandler();
    }
    if (PC->isCatchLast()) {
      return popCaught();
    }
    return PC;
  }

  // Unsafe leave try-block or catch-block
  AST::InstrView::iterator
  popHandlerOrCaught(AST::InstrView::iterator PC) noexcept {
    if (PC->isTryLast()) {
      return popHandler();
    } else {
      return popCaught();
    }
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
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> ValueStack;
  std::vector<Frame> FrameStack;
  std::vector<Handler> HandlerStack;
  std::vector<Exception> CaughtStack;
  /// @}
};

} // namespace Runtime
} // namespace WasmEdge
