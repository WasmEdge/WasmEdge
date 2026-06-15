// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
#include "common/types.h"
#include "gc/allocator.h"
#include "runtime/instance/module.h"

#include <cstring>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// The SIMD element types stored on the value stack and popped through tuples.
#define WASMEDGE_FOR_EACH_SIMD_TYPE(X)                                         \
  X(int64x2_t)                                                                 \
  X(uint64x2_t)                                                                \
  X(int32x4_t)                                                                 \
  X(uint32x4_t)                                                                \
  X(int16x8_t)                                                                 \
  X(uint16x8_t) X(int8x16_t) X(uint8x16_t) X(doublex2_t) X(floatx4_t)

// clang-cl gives alignof(std::tuple<uint64x2_t>) 8 not 16, faulting aligned
// SIMD loads on a tuple slot. Over-align MS-STL's per-element _Tuple_val<T> to
// restore 16-byte alignment so StackManager can use plain std::tuple. clang-cl
// only (native Clang aligns; MSVC uses std::array).
// Ref: https://github.com/llvm/llvm-project/issues/134694
#if defined(_MSC_VER) && defined(__clang__)
namespace std {
#define WASMEDGE_X(T)                                                          \
  template <> struct alignas(WasmEdge::T) _Tuple_val<WasmEdge::T> {            \
    constexpr _Tuple_val() : _Val() {}                                         \
    template <typename OtherT>                                                 \
    constexpr _Tuple_val(OtherT &&Arg) : _Val(std::forward<OtherT>(Arg)) {}    \
    WasmEdge::T _Val;                                                          \
  };
WASMEDGE_FOR_EACH_SIMD_TYPE(WASMEDGE_X)
#undef WASMEDGE_X
} // namespace std
#endif

// Outside the clang-cl guard above so any future alignment regression fails
// here at compile time, not at runtime.
#define WASMEDGE_X(T)                                                          \
  static_assert(alignof(std::tuple<WasmEdge::T>) == alignof(WasmEdge::T),      \
                "tuple<" #T "> alignment mismatch");
WASMEDGE_FOR_EACH_SIMD_TYPE(WASMEDGE_X)
#undef WASMEDGE_X
#undef WASMEDGE_FOR_EACH_SIMD_TYPE

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
  explicit StackManager(GC::Allocator &A) noexcept : Allocator(A) {
    ValueStack.reserve(2048U);
    FrameStack.reserve(16U);
    Allocator.addStack(ValueStack);
  }
  ~StackManager() noexcept { Allocator.removeStack(ValueStack); }

  // ValueStack is GC-registered by address (addStack); a copy/move would leave
  // the new stack unregistered (refs invisible -> premature reclaim) and make
  // the destructor's removeStack target an unknown address.
  StackManager(const StackManager &) = delete;
  StackManager(StackManager &&) = delete;
  StackManager &operator=(const StackManager &) = delete;
  StackManager &operator=(StackManager &&) = delete;

  /// Getter for stack size.
  size_t size() const noexcept { return ValueStack.size(); }

  /// Push a new value entry. noexcept by policy: on a growth-path allocation
  /// failure, abort rather than unwind -- the GC-registered stack must not be
  /// left partially grown mid-collection.
  ///
  /// Unlike emplace(), copies Val verbatim without zeroing a numeric's high
  /// word. The conservative GC reads every slot's high word as a candidate
  /// pointer, so callers must pass a determinate high word (a ref's pointer, or
  /// zero for numerics); push cannot sanitize as it cannot tell a ref pointer
  /// from numeric bits. The numeric producers feeding this path (ValueFromType,
  /// emplaceAddr, Instruction immediates, host param/return conv) zero-extend.
  template <typename T> void push(T &&Val) noexcept {
    // Reallocating past capacity frees the buffer the collector's root scan may
    // be iterating; lock only on that rare growth path (see lockStackRoots) --
    // the common in-capacity push stays lock-free.
    if (unlikely(ValueStack.size() == ValueStack.capacity())) {
      auto Lock = Allocator.lockStackRoots();
      ValueStack.push_back(std::forward<T>(Val));
    } else {
      ValueStack.push_back(std::forward<T>(Val));
    }
  }

  /// Pop and return the top entry.
  template <typename T> T pop() noexcept {
    assuming(!ValueStack.empty());
    if constexpr (std::is_same_v<detail::remove_cvref_t<T>, RefVariant>) {
      // A ref popped into a C++ local leaves the rooted stack; shade it so a
      // concurrent collection keeps it alive until the caller is done (e.g. the
      // struct.set/array.set destination ref used after this pop). Numeric pops
      // carry no ref.
      Allocator.writeBarrier(ValueStack.back());
    }
    Value V = std::move(ValueStack.back());
    ValueStack.pop_back();
    return get<T>(V);
  }

  /// Pop the top entries into a tuple; first template argument is the topmost.
  /// Brace-init evaluates left-to-right, so pops run in template order.
  template <typename... ArgsT> std::tuple<ArgsT...> pops() noexcept {
    return std::tuple<ArgsT...>{pop<ArgsT>()...};
  }

  /// Getter of top entry of stack.
  template <typename T> T peekTop() const noexcept {
    assuming(!ValueStack.empty());
    return get<T>(ValueStack.back());
  }

  /// Getter of top N-th value entry of stack.
  template <typename T> T peekTopN(uint32_t Offset) const noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    return get<T>(ValueStack[ValueStack.size() - Offset]);
  }

  /// Pop the I-th operand, or peek (leave on the stack) the last one.
  template <std::size_t I, std::size_t Last, typename Ty>
  Ty popOrPeek() noexcept {
    if constexpr (I == Last) {
      return peekTop<Ty>();
    } else {
      return pop<Ty>();
    }
  }
  template <typename... ArgsT, std::size_t... Is>
  std::tuple<ArgsT...> popsPeekTopImpl(std::index_sequence<Is...>) noexcept {
    return std::tuple<ArgsT...>{
        popOrPeek<Is, sizeof...(ArgsT) - 1, ArgsT>()...};
  }
  /// Pop the top entries into a tuple but leave the last on the stack (peeked),
  /// keeping a ref-typed survivor rooted while shallower operands pop. First
  /// template argument is the topmost entry; the last is the peeked survivor.
  template <typename T, typename... ArgsT>
  std::tuple<T, ArgsT...> popsPeekTop() noexcept {
    return popsPeekTopImpl<T, ArgsT...>(std::index_sequence_for<T, ArgsT...>{});
  }

  /// Setter of top value entry of stack.
  template <typename T> void emplaceTop(T &&Val) noexcept {
    assuming(!ValueStack.empty());
    emplace(ValueStack.back(), std::forward<T>(Val));
  }

  /// Setter of top N-th value entry of stack.
  template <typename T> void emplaceTopN(uint32_t Offset, T &&Val) noexcept {
    assuming(0 < Offset && Offset <= ValueStack.size());
    emplace(ValueStack[ValueStack.size() - Offset], std::forward<T>(Val));
  }

  /// Push a span of values to the stack.
  void pushSpan(Span<const Value> ValSpan) noexcept {
    // See push(): hold the allocator's stack lock only when the insert grows
    // past capacity and reallocates the buffer the collector may be scanning.
    if (unlikely(ValueStack.size() + ValSpan.size() > ValueStack.capacity())) {
      auto Lock = Allocator.lockStackRoots();
      ValueStack.insert(ValueStack.end(), ValSpan.begin(), ValSpan.end());
    } else {
      ValueStack.insert(ValueStack.end(), ValSpan.begin(), ValSpan.end());
    }
  }

  /// Pop the top ValSpan.size() entries into ValSpan. ValSpan[0] receives the
  /// deepest (earliest-pushed) of the popped entries, ValSpan.back() the top.
  void popSpan(Span<Value> ValSpan) noexcept {
    assuming(ValSpan.size() <= ValueStack.size());
    // The popped entries leave the rooted stack into the caller's buffer; shade
    // them before the erase so an in-flight collection keeps them alive.
    shadeStackRootsFrom(ValueStack.size() - ValSpan.size());
    const auto VSBegin =
        ValueStack.end() - static_cast<ptrdiff_t>(ValSpan.size());
    std::move(VSBegin, ValueStack.end(), ValSpan.begin());
    ValueStack.erase(VSBegin, ValueStack.end());
  }

  /// View of the top N entries while they remain on the stack. Used at the
  /// host/compiled-function boundary to keep args on the GC-rooted stack for
  /// the call: a detached buffer would unroot their refs while the callee runs.
  /// The span aliases the stack buffer and dangles after any growth
  /// (push/pushSpan realloc); do not hold it across such operations.
  Span<Value> getTopSpan(uint32_t N) noexcept {
    assuming(N <= ValueStack.size());
    return Span<Value>(ValueStack.end() - static_cast<ptrdiff_t>(N), N);
  }

  /// Push a new frame entry to the stack.
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
      // Moving erase relocates the preserved Locals tail to lower slots; shade
      // first so a concurrent root scan cannot miss a relocated ref (see
      // shadeStackRootsFrom).
      shadeStackRootsFrom(FrameStack.back().VPos - FrameStack.back().Locals);
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

  /// Pop top frame.
  AST::InstrView::iterator popFrame() noexcept {
    assuming(!FrameStack.empty());
    assuming(FrameStack.back().VPos >= FrameStack.back().Locals);
    assuming(FrameStack.back().VPos - FrameStack.back().Locals <=
             ValueStack.size() - FrameStack.back().Arity);
    // Moving erase relocates the Arity result tail to lower slots; shade first
    // (see shadeStackRootsFrom).
    shadeStackRootsFrom(FrameStack.back().VPos - FrameStack.back().Locals);
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
        // Moving erase relocates the AssocValSize tail; shade first (see
        // shadeStackRootsFrom).
        shadeStackRootsFrom(TopHandler.VPos);
        ValueStack.erase(ValueStack.begin() + TopHandler.VPos,
                         ValueStack.end() - AssocValSize);
        return TopHandler;
      }
      FrameStack.pop_back();
    }
    return std::nullopt;
  }

  /// Remove inactive handler.
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

  /// Erase value stack.
  void eraseValueStack(uint32_t EraseBegin, uint32_t EraseEnd) noexcept {
    assuming(EraseEnd <= EraseBegin && EraseBegin <= ValueStack.size());
    // Moving erase (EraseEnd > 0) relocates the preserved tail; shade first
    // (see shadeStackRootsFrom).
    shadeStackRootsFrom(ValueStack.size() - EraseBegin);
    ValueStack.erase(ValueStack.end() - EraseBegin,
                     ValueStack.end() - EraseEnd);
  }

  // Get all Value
  Span<const Value> getValueSpan() const { return ValueStack; }

  /// Leave top label.
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

  /// Getter of module address.
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
  /// \name Data of the stack manager.
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
    using U = detail::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, Value>) {
      V = std::forward<T>(Val);
    } else {
      if constexpr (sizeof(U) < sizeof(Value)) {
        // Clear the slot before writing a value narrower than ValVariant: the
        // conservative GC reads each slot's high word as a candidate pointer,
        // so stale bits from a previously stored ref (e.g. a number written
        // over a ref by local.set) would be a false root. ValVariant is
        // trivially copyable, so zeroing raw storage first is well-defined.
        std::memset(static_cast<void *>(&V), 0, sizeof(V));
      }
      V.emplace<U>(std::forward<T>(Val));
    }
  }

  // SATB shading for value-stack roots. Before an erase removes or relocates
  // slots, shade the refs in [FromIdx, size) so an in-flight collection cannot
  // miss them: a moving erase relocates live tail refs to lower slots without
  // the stack-root lock, and a root scan that already passed a slot never
  // revisits it. Marking gray up front means they survive regardless of the
  // move. bulkWriteBarrier self-guards on collector state (one atomic load when
  // idle).
  void shadeStackRootsFrom(size_t FromIdx) noexcept {
    if (FromIdx < ValueStack.size()) {
      Allocator.bulkWriteBarrier(Span<const Value>(
          ValueStack.data() + FromIdx, ValueStack.size() - FromIdx));
    }
  }
};

} // namespace Runtime
} // namespace WasmEdge
