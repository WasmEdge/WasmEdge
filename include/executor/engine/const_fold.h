// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/executor/engine/const_fold.h - Constant folding ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the interpreter constant folding pass
/// and its associated cost model.
///
/// ## Cost Model Background
///
/// This optimization pass uses a stack-thrash-aware cost model inspired by the
/// Luau compiler's approach, adapted for WasmEdge's interpreter architecture.
///
/// Reference cost models from production compilers:
///
/// GCC (gcc/ipa-inline-analysis.cc): Uses instruction count as primary cost
///   metric. Functions under ~40 instructions are auto-inlined. Cost is
///   weighted by loop nesting (10x per level).
///
/// LLVM (llvm/include/llvm/Analysis/InlineCost.h): InlineCost class with
///   default threshold of 225 cost units. Each LLVM IR instruction costs 5
///   units base. Memory ops and calls cost more. Bonus for single-callsite
///   and small functions.
///
/// ChakraCore (lib/Backend/InliningDecider.cpp): Uses bytecode size + call
///   depth + loop presence. Threshold scales with optimization level.
///   Small functions (<30 bytecodes) inlined aggressively.
///
/// Luau (Compiler/src/CostModel.cpp): Packed 64-bit parallel cost model.
///   Byte 0 = baseline cost, bytes 1-7 = discount when variable 0-6 is
///   constant. Uses saturating byte arithmetic to compute all 8 scenarios
///   simultaneously. Literals cost 0, calls cost 3, allocations cost 10.
///
/// ## WasmEdge Stack-Thrash Cost Model
///
/// WasmEdge's interpreter uses std::vector<ValVariant> as its operand stack.
/// Each push_back/pop_back touches the vector's size field and data pointer
/// (at minimum 2 cache lines on x86-64), and the dispatch loop requires a
/// switch + indirect call per instruction. The cost model accounts for both
/// dispatch overhead and stack traffic to determine fold profitability.
///
/// Cost constants (abstract units):
///   DispatchCost  = 1  // switch lookup + function call per instruction
///   StackPushCost = 2  // vector push_back: size check + store + increment
///   StackPopCost  = 1  // move + pop_back
///   NopCost       = 1  // residual dispatch cost for nop placeholder
///
/// Per-pattern savings (before -> after = net saved):
///   const+const+binop -> const+nop+nop : 8 -> 5 = net 3 saved
///   const+unary       -> const+nop     : 4 -> 4 = net 0 (saves compute)
///   const+const+relop -> i32.const+nop+nop : same as binary, net 3 saved
///   identity elim     -> nop+nop       : 5 -> 2 = net 3 saved
///
/// Since all patterns yield non-negative savings, the fold threshold is 0:
/// we fold unconditionally for all operations that preserve Wasm semantics.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/enum_ast.hpp"
#include "common/types.h"

#include <vector>

namespace WasmEdge {

namespace AST {
class Instruction;
class Module;
using InstrVec = std::vector<Instruction>;
} // namespace AST

namespace Executor {

/// Cost model constants for the WasmEdge interpreter.
/// These quantify the overhead of each interpreter operation in abstract
/// units, used to determine when constant folding is profitable.
namespace CostModel {
// NOLINTBEGIN(readability-identifier-naming)
constexpr uint32_t DispatchCost = 1;
constexpr uint32_t StackPushCost = 2;
constexpr uint32_t StackPopCost = 1;
constexpr uint32_t NopCost = 1;

/// Cost of a const instruction: dispatch + push.
constexpr uint32_t ConstCost = DispatchCost + StackPushCost;
/// Cost of a binary op: dispatch + pop(rhs). LHS modified in-place via
/// getTop() reference, so no additional push/pop.
constexpr uint32_t BinaryOpCost = DispatchCost + StackPopCost;
/// Cost of a unary op: dispatch only. Operates on getTop() in-place.
constexpr uint32_t UnaryOpCost = DispatchCost;
// NOLINTEND(readability-identifier-naming)
} // namespace CostModel

/// Run constant folding optimization on an instruction vector.
///
/// This pass identifies patterns where constant operands flow into pure
/// arithmetic/comparison/cast operations and evaluates them at load time,
/// replacing the original instruction sequence with a single const instruction
/// and Nop fillers. Nop-filling preserves all jump offsets (JumpEnd, JumpElse,
/// PCOffset, BrTable labels) since the vector size is unchanged.
///
/// The pass runs a single forward scan with back-tracking on successful folds
/// to enable chained folding (e.g., const+const+add followed by const+mul
/// becomes const+const+mul after the first fold, then folds again).
///
/// Safety: Division/remainder with zero divisor or signed overflow are never
/// folded (the runtime trap must fire). Non-saturating float-to-int truncation
/// with NaN/Inf/out-of-range inputs is never folded. All other operations are
/// safe to fold because Wasm arithmetic is fully specified (no UB).
void optimizeConstantExpressions(AST::InstrVec &Instrs);

/// Run the full interprocedural constant propagation (IPCP) pipeline on a
/// parsed Wasm module, in-place, before instantiation.
///
/// The pipeline performs:
///   1. Purity analysis — determines which functions are free of observable
///      side effects (no memory/global/table access, no indirect calls, no
///      exception ops).  Imported functions are conservatively marked impure.
///      Propagated via fixpoint through the call graph.
///   2. Arithmetic constant folding — `optimizeConstantExpressions` on each
///      function body (const+const+binop → const+nop+nop, etc.).
///   3. Call-site folding — for each `call` whose actual arguments are all
///      statically-known constants AND whose callee is pure, evaluate the
///      callee with a bounded mini interpreter and replace the
///      const…const+call sequence with the result const(s).
///   4. Iterate 2–3 to fixpoint per function (a folded call result may unlock
///      further arithmetic folds or nested call folds).
///
/// This collapses programs like `fib(10)` and `fac(10)` (called from main()
/// with literal arguments) to a single `i32.const` at module load time.
///
/// Safety: The mini interpreter respects all Wasm safety invariants — it never
/// evaluates impure functions, never follows indirect calls, and respects the
/// step/depth budget to guarantee bounded load-time overhead.
///
/// The StepBudget and DepthBudget parameters allow callers to control the
/// maximum work the pass will do.  Defaults are generous (4M steps, 500
/// recursive depth).  Pass 0 for StepBudget to skip IPCP entirely (arithmetic
/// folding still runs).
void optimizeModuleConstantExpressions(AST::Module &Mod,
                                       uint32_t StepBudget = 4000000,
                                       uint32_t DepthBudget = 500) noexcept;

} // namespace Executor
} // namespace WasmEdge
