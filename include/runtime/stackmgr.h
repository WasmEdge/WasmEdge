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
#include "gc/allocator.h"
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
        : Module(Mod), From(FromIt), Arity(A), VPos(V), Locals(L) {}
    const Instance::ModuleInstance *Module;
    AST::InstrView::iterator From;
    uint32_t Arity;
    uint32_t VPos;
    uint32_t Locals;
    std::vector<Handler> HandlerStack;
  };

  /// Stack manager provides the stack control for Wasm execution with VALIDATED
  /// modules. All operations of instructions passed validation, therefore no
  /// unexpect operations will occur.
  StackManager(GC::Allocator &A) noexcept : Allocator(A) {
    ValueStack.reserve(2048U);
    FrameStack.reserve(16U);
    Allocator.addStack(ValueStack);
  }
  ~StackManager() noexcept { Allocator.removeStack(ValueStack); }

  /// Getter of stack size.
  size_t unsafeSize() const noexcept { return ValueStack.size(); }
  size_t size() const noexcept { return unsafeSize(); }

  /// Push a new value entry to stack.
  template <typename T> void unsafePush(T &&Val) noexcept {
    ValueStack.push_back(std::forward<T>(Val));
  }
  template <typename T> void push(T &&Val) noexcept {
    return unsafePush(std::forward<T>(Val));
  }

  template <typename... ArgsT> void unsafePushs(ArgsT &&...Vals) noexcept {
    (unsafePush(std::forward<ArgsT>(Vals)), ...);
  }
  template <typename... ArgsT> void pushs(ArgsT &&...Vals) noexcept {
    return unsafePushs(std::forward<ArgsT>(Vals)...)();
  }

  /// Unsafe pop and return the top entry.
  template <typename T> T unsafePop() noexcept {
    assuming(!ValueStack.empty());
    const Value V = std::move(ValueStack.back());
    ValueStack.pop_back();
    return get<T>(V);
  }
  template <typename T> T pop() noexcept { return unsafePop<T>(); }

  /// Unsafe pop and return the top entry.
  template <typename... ArgsT> std::tuple<ArgsT...> unsafePops() noexcept {
    std::tuple<ArgsT...> Ret{unsafePop<ArgsT>()...};
    return Ret;
  }
  template <typename... ArgsT> std::tuple<ArgsT...> pops() noexcept {
    return unsafePops<ArgsT...>();
  }

  /// Unsafe getter of top entry of stack.
  template <typename T> T unsafePeekTop() const noexcept {
    assuming(!ValueStack.empty());
    return get<T>(ValueStack.back());
  }
  template <typename T> T peekTop() const noexcept {
    return unsafePeekTop<T>();
  }

  /// Unsafe getter of top N-th value entry of stack.
  template <typename T> T unsafePeekTopN(uint32_t Offset) noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    return get<T>(ValueStack[ValueStack.size() - Offset]);
  }
  template <typename T> T peekTopN(uint32_t Offset) noexcept {
    return unsafePeekTopN<T>(Offset);
  }

  /// Unsafe getter of top entry of stack.
  template <typename T, typename... ArgsT>
  std::tuple<T, ArgsT...> unsafePopsPeekTop() noexcept {
    if constexpr (sizeof...(ArgsT) == 0) {
      return std::make_tuple(unsafePeekTop<T>());
    } else {
      std::tuple<T> Val{unsafePops<T>()};
      return std::tuple_cat(std::move(Val), unsafePopsPeekTop<ArgsT...>());
    }
  }
  template <typename... ArgsT> std::tuple<ArgsT...> popsPeekTop() noexcept {
    return unsafePopsPeekTop<ArgsT...>();
  }

  template <typename T> void unsafeEmplaceTop(T &&Val) noexcept {
    emplace(ValueStack.back(), std::forward<T>(Val));
  }
  template <typename T> void emplaceTop(T &&Val) noexcept {
    return unsafeEmplaceTop(std::forward<T>(Val));
  }

  /// Unsafe getter of top N-th value entry of stack.
  template <typename T>
  void unsafeEmplaceTopN(uint32_t Offset, T &&Val) noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    emplace(ValueStack[ValueStack.size() - Offset], std::forward<T>(Val));
  }
  template <typename T> void emplaceTopN(uint32_t Offset, T &&Val) noexcept {
    return unsafeEmplaceTopN<T>(Offset, std::forward<T>(Val));
  }

  /// Push a span of value to stack
  void unsafePushSpan(Span<const Value> ValSpan) noexcept {
    ValueStack.insert(ValueStack.end(), ValSpan.begin(), ValSpan.end());
  }
  void pushSpan(Span<const Value> ValSpan) noexcept {
    return unsafePushSpan(ValSpan);
  }

  /// Unsafe pop and return the top N entries.
  void unsafePopSpan(Span<Value> ValSpan) noexcept {
    assuming(ValSpan.size() <= ValueStack.size());
    const auto VSBegin =
        ValueStack.end() - static_cast<ptrdiff_t>(ValSpan.size());
    std::move(VSBegin, ValueStack.end(), ValSpan.begin());
    ValueStack.erase(VSBegin, ValueStack.end());
  }
  void popSpan(Span<Value> ValSpan) noexcept { return unsafePopSpan(ValSpan); }

  /// Unsafe pop and return the top N entries.
  std::vector<Value> unsafePopVec(uint32_t N) {
    std::vector<Value> Vec(N);
    unsafePopSpan(Vec);
    return Vec;
  }
  std::vector<Value> popVec(uint32_t N) { return unsafePopVec(N); }

  /// Push a new frame entry to stack.
  void pushFrame(const Instance::ModuleInstance *Module,
                 AST::InstrView::iterator From, uint32_t Locals = 0,
                 uint32_t Arity = 0, bool IsTailCall = false) noexcept {
    if (!IsTailCall) {
      FrameStack.emplace_back(Module, From, Locals, Arity,
                              static_cast<uint32_t>(ValueStack.size()));
    } else {
      assuming(!FrameStack.empty());
      assuming(FrameStack.back().VPos >= FrameStack.back().Locals);
      assuming(FrameStack.back().VPos - FrameStack.back().Locals <=
               ValueStack.size() - Locals);
      ValueStack.erase(ValueStack.begin() +
                           (FrameStack.back().VPos - FrameStack.back().Locals),
                       ValueStack.end() - Locals);
      FrameStack.back().Module = Module;
      FrameStack.back().Locals = Locals;
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
    ValueStack.erase(ValueStack.begin() +
                         (FrameStack.back().VPos - FrameStack.back().Locals),
                     ValueStack.end() - FrameStack.back().Arity);
    auto From = FrameStack.back().From;
    FrameStack.pop_back();
    return From;
  }

  // Get all frames
  Span<const Frame> getFramesSpan() const { return FrameStack; }

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

  // Get all Value
  template <typename CallbackT> void getValueSpan(CallbackT &&C) const {
    std::forward<CallbackT>(C)(Span<const Value>{ValueStack});
    return;
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
    if (unlikely(FrameStack.empty())) {
      return nullptr;
    }
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
  GC::Allocator &Allocator;
  /// @}
  template <typename T> static T get(const Value &V) noexcept {
    if constexpr (std::is_same_v<detail::remove_cvref_t<T>, Value>) {
      return V;
    } else {
      return V.get<T>();
    }
  }
  template <typename T> static void emplace(Value &V, T &&Val) noexcept {
    if constexpr (std::is_same_v<detail::remove_cvref_t<T>, Value>) {
      V = Val;
    } else {
      V.emplace<detail::remove_cvref_t<T>>(std::forward<T>(Val));
    }
  }
};

} // namespace Runtime
} // namespace WasmEdge
