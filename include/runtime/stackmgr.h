// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

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

#include <optional>
#include <vector>

namespace WasmEdge {
namespace Runtime {

class StackManager {
public:
  using Value = ValVariant;

  struct Handler {
    Handler(AST::InstrView::iterator TryIt, uint32_t V,
            Span<const AST::Instruction::CatchDescriptor> C)
        : Try(TryIt), VPos(V), CatchClause(C) {}
    AST::InstrView::iterator Try;
    uint32_t VPos;
    Span<const AST::Instruction::CatchDescriptor> CatchClause;
  };

  struct Frame {
    Frame() = delete;
    Frame(const Instance::ModuleInstance *Mod, AST::InstrView::iterator FromIt,
          uint32_t L, uint32_t A, uint32_t V) noexcept
        : Module(Mod), From(FromIt), Locals(L), Arity(A), VPos(V) {}
    const Instance::ModuleInstance *Module;
    AST::InstrView::iterator From;
    uint32_t Locals;
    uint32_t Arity;
    uint32_t VPos;
    std::vector<Handler> HandlerStack;
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

  /// Unsafe pop and return the top entry.
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
    if (!IsTailCall) {
      FrameStack.emplace_back(Module, From, LocalNum, Arity,
                              static_cast<uint32_t>(ValueStack.size()));
    } else {
      assuming(!FrameStack.empty());
      assuming(FrameStack.back().VPos >= FrameStack.back().Locals);
      assuming(FrameStack.back().VPos - FrameStack.back().Locals <=
               ValueStack.size() - LocalNum);
      ValueStack.erase(ValueStack.begin() + FrameStack.back().VPos -
                           FrameStack.back().Locals,
                       ValueStack.end() - LocalNum);
      FrameStack.back().Module = Module;
      FrameStack.back().Locals = LocalNum;
      FrameStack.back().Arity = Arity;
      FrameStack.back().VPos = static_cast<uint32_t>(ValueStack.size());
      FrameStack.back().HandlerStack.clear();
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
    assuming(!FrameStack.empty());
    FrameStack.back().HandlerStack.emplace_back(
        TryIt, static_cast<uint32_t>(ValueStack.size()) - BlockParamNum, Catch);
  }

  /// Pop the top handler on the stack.
  std::optional<Handler> popTopHandler(uint32_t AssocValSize) noexcept {
    while (!FrameStack.empty()) {
      auto &Frame = FrameStack.back();
      if (!Frame.HandlerStack.empty()) {
        auto TopHandler = std::move(Frame.HandlerStack.back());
        Frame.HandlerStack.pop_back();
        assuming(TopHandler.VPos <= ValueStack.size() - AssocValSize);
        ValueStack.erase(ValueStack.begin() + TopHandler.VPos,
                         ValueStack.end() - AssocValSize);
        return TopHandler;
      }
      FrameStack.pop_back();
    }
    return std::nullopt;
  }

  /// Unsafe remove inactive handler.
  void removeInactiveHandler(AST::InstrView::iterator PC) noexcept {
    assuming(!FrameStack.empty());
    // First pop the inactive handlers. Br instructions may cause the handlers
    // in current frame becomes inactive.
    auto &HandlerStack = FrameStack.back().HandlerStack;
    while (!HandlerStack.empty()) {
      auto &Handler = HandlerStack.back();
      if (PC < Handler.Try ||
          PC > Handler.Try + Handler.Try->getTryCatch().JumpEnd) {
        HandlerStack.pop_back();
      } else {
        break;
      }
    }
  }

  /// Unsafe erase value stack.
  void eraseValueStack(uint32_t EraseBegin, uint32_t EraseEnd) noexcept {
    assuming(EraseEnd <= EraseBegin && EraseBegin <= ValueStack.size());
    ValueStack.erase(ValueStack.end() - EraseBegin,
                     ValueStack.end() - EraseEnd);
  }

  /// Unsafe leave top label.
  AST::InstrView::iterator
  maybePopFrameOrHandler(AST::InstrView::iterator PC) noexcept {
    if (FrameStack.size() > 1 && PC->isExprLast()) {
      // Noted that there's always a base frame in stack.
      return popFrame();
    }
    if (PC->isTryBlockLast()) {
      FrameStack.back().HandlerStack.pop_back();
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
  }

private:
  /// \name Data of stack manager.
  /// @{
  std::vector<Value> ValueStack;
  std::vector<Frame> FrameStack;
  /// @}
};

} // namespace Runtime
} // namespace WasmEdge
