// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/interpreter/interpreter.h - Interpreter class definition -===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Interpreter class, which
/// instantiate and run Wasm modules.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/instruction.h"
#include "ast/module.h"
#include "common/defines.h"
#include "common/statistics.h"
#include "runtime/importobj.h"
#include "runtime/stackmgr.h"
#include "runtime/storemgr.h"

#include <cassert>
#include <csignal>
#include <memory>
#include <type_traits>
#include <vector>

namespace WasmEdge {
namespace Interpreter {

namespace {

/// Template return type aliasing
/// Accept unsigned integer types. (uint32_t, uint64_t)
template <typename T>
using TypeU = typename std::enable_if_t<IsWasmUnsignV<T>, Expect<void>>;
/// Accept integer types. (uint32_t, int32_t, uint64_t, int64_t)
template <typename T>
using TypeI = typename std::enable_if_t<IsWasmIntV<T>, Expect<void>>;
/// Accept floating types. (float, double)
template <typename T>
using TypeF = typename std::enable_if_t<IsWasmFloatV<T>, Expect<void>>;
/// Accept all num types. (uint32_t, int32_t, uint64_t, int64_t, float, double)
template <typename T>
using TypeT = typename std::enable_if_t<IsWasmNumV<T>, Expect<void>>;
/// Accept Wasm built-in num types. (uint32_t, uint64_t, float, double)
template <typename T>
using TypeN = typename std::enable_if_t<IsWasmNativeNumV<T>, Expect<void>>;

/// Accept (unsigned integer types, unsigned integer types).
template <typename T1, typename T2>
using TypeUU = typename std::enable_if_t<IsWasmUnsignV<T1> && IsWasmUnsignV<T2>,
                                         Expect<void>>;
/// Accept (integer types, unsigned integer types).
template <typename T1, typename T2>
using TypeIU = typename std::enable_if_t<IsWasmIntV<T1> && IsWasmUnsignV<T2>,
                                         Expect<void>>;
/// Accept (floating types, floating types).
template <typename T1, typename T2>
using TypeFF = typename std::enable_if_t<IsWasmFloatV<T1> && IsWasmFloatV<T2>,
                                         Expect<void>>;
/// Accept (integer types, floating types).
template <typename T1, typename T2>
using TypeIF =
    typename std::enable_if_t<IsWasmIntV<T1> && IsWasmFloatV<T2>, Expect<void>>;
/// Accept (floating types, integer types).
template <typename T1, typename T2>
using TypeFI =
    typename std::enable_if_t<IsWasmFloatV<T1> && IsWasmIntV<T2>, Expect<void>>;
/// Accept (Wasm built-in num types, Wasm built-in num types).
template <typename T1, typename T2>
using TypeNN =
    typename std::enable_if_t<IsWasmNativeNumV<T1> && IsWasmNativeNumV<T2> &&
                                  sizeof(T1) == sizeof(T2),
                              Expect<void>>;

} // namespace

/// Executor flow control class.
class Interpreter {
public:
  Interpreter(const Configure &Conf,
              Statistics::Statistics *S = nullptr) noexcept
      : Conf(Conf), Stat(S) {
    assert(This == nullptr);
    This = this;
    if (Stat) {
      ExecutionContext.InstrCount = &Stat->getInstrCountRef();
      ExecutionContext.CostTable = Stat->getCostTable().data();
      ExecutionContext.Gas = &Stat->getTotalCostRef();
    }
  }
  ~Interpreter() noexcept { This = nullptr; }

  /// Instantiate Wasm Module as the anonymous active module.
  Expect<void> instantiateModule(Runtime::StoreManager &StoreMgr,
                                 const AST::Module &Mod);

  /// Register host module.
  Expect<void> registerModule(Runtime::StoreManager &StoreMgr,
                              const Runtime::ImportObject &Obj);

  /// Register Wasm module.
  Expect<void> registerModule(Runtime::StoreManager &StoreMgr,
                              const AST::Module &Mod, std::string_view Name);

  /// Invoke function by function address in Store manager.
  Expect<std::vector<ValVariant>> invoke(Runtime::StoreManager &StoreMgr,
                                         const uint32_t FuncAddr,
                                         Span<const ValVariant> Params,
                                         Span<const ValType> ParamTypes);

private:
  /// Run Wasm bytecode expression for initialization.
  Expect<void> runExpression(Runtime::StoreManager &StoreMgr,
                             AST::InstrView Instrs);

  /// Run Wasm function.
  Expect<void> runFunction(Runtime::StoreManager &StoreMgr,
                           const Runtime::Instance::FunctionInstance &Func,
                           Span<const ValVariant> Params);

  /// Execute instructions.
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::InstrView::iterator Start,
                       const AST::InstrView::iterator End);

  /// \name Functions for instantiation.
  /// @{
  /// Instantiation of Module Instance.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           const AST::Module &Mod, std::string_view Name);

  /// Instantiation of Import Section.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::ImportSection &ImportSec);

  /// Instantiation of Function Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::FunctionSection &FuncSec,
                           const AST::CodeSection &CodeSec);

  /// Instantiation of Global Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::GlobalSection &GlobSec);

  /// Instantiation of Table Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::TableSection &TabSec);

  /// Instantiation of Memory Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::MemorySection &MemSec);

  /// Instantiation of Element Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::ElementSection &ElemSec);

  /// Initialize table with Element Instances.
  Expect<void> initTable(Runtime::StoreManager &StoreMgr,
                         Runtime::Instance::ModuleInstance &ModInst,
                         const AST::ElementSection &ElemSec);

  /// Initialize memory with Data Instances.
  Expect<void> initMemory(Runtime::StoreManager &StoreMgr,
                          Runtime::Instance::ModuleInstance &ModInst,
                          const AST::DataSection &DataSec);

  /// Instantiation of Data Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::DataSection &DataSec);

  /// Instantiation of Export Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::ExportSection &ExportSec);
  /// @}

  /// \name Helper Functions for block controls.
  /// @{
  /// Helper function for calling functions. Return the continuation iterator.
  Expect<AST::InstrView::iterator>
  enterFunction(Runtime::StoreManager &StoreMgr,
                const Runtime::Instance::FunctionInstance &Func,
                const AST::InstrView::iterator From);

  /// Helper function for branching to label.
  Expect<void> branchToLabel(Runtime::StoreManager &StoreMgr,
                             const uint32_t Cnt, AST::InstrView::iterator &PC);

  /// Helper function for getting arity from block type.
  std::pair<uint32_t, uint32_t> getBlockArity(Runtime::StoreManager &StoreMgr,
                                              const BlockType &BType);
  /// @}

  /// \name Helper Functions for getting instances.
  /// @{
  /// Helper function for get table instance by index.
  Runtime::Instance::TableInstance *
  getTabInstByIdx(Runtime::StoreManager &StoreMgr, const uint32_t Idx);

  /// Helper function for get memory instance by index.
  Runtime::Instance::MemoryInstance *
  getMemInstByIdx(Runtime::StoreManager &StoreMgr, const uint32_t Idx);

  /// Helper function for get global instance by index.
  Runtime::Instance::GlobalInstance *
  getGlobInstByIdx(Runtime::StoreManager &StoreMgr, const uint32_t Idx);

  /// Helper function for get element instance by index.
  Runtime::Instance::ElementInstance *
  getElemInstByIdx(Runtime::StoreManager &StoreMgr, const uint32_t Idx);

  /// Helper function for get data instance by index.
  Runtime::Instance::DataInstance *
  getDataInstByIdx(Runtime::StoreManager &StoreMgr, const uint32_t Idx);
  /// @}

  /// \name Run instructions functions
  /// @{
  /// ======= Control instructions =======
  Expect<void> runBlockOp(Runtime::StoreManager &StoreMgr,
                          const AST::Instruction &Instr,
                          AST::InstrView::iterator &PC);
  Expect<void> runLoopOp(Runtime::StoreManager &StoreMgr,
                         const AST::Instruction &Instr,
                         AST::InstrView::iterator &PC);
  Expect<void> runIfElseOp(Runtime::StoreManager &StoreMgr,
                           const AST::Instruction &Instr,
                           AST::InstrView::iterator &PC);
  Expect<void> runBrOp(Runtime::StoreManager &StoreMgr,
                       const AST::Instruction &Instr,
                       AST::InstrView::iterator &PC);
  Expect<void> runBrIfOp(Runtime::StoreManager &StoreMgr,
                         const AST::Instruction &Instr,
                         AST::InstrView::iterator &PC);
  Expect<void> runBrTableOp(Runtime::StoreManager &StoreMgr,
                            const AST::Instruction &Instr,
                            AST::InstrView::iterator &PC);
  Expect<void> runReturnOp(AST::InstrView::iterator &PC);
  Expect<void> runCallOp(Runtime::StoreManager &StoreMgr,
                         const AST::Instruction &Instr,
                         AST::InstrView::iterator &PC);
  Expect<void> runCallIndirectOp(Runtime::StoreManager &StoreMgr,
                                 const AST::Instruction &Instr,
                                 AST::InstrView::iterator &PC);
  /// ======= Variable instructions =======
  Expect<void> runLocalGetOp(const uint32_t Idx);
  Expect<void> runLocalSetOp(const uint32_t Idx);
  Expect<void> runLocalTeeOp(const uint32_t Idx);
  Expect<void> runGlobalGetOp(Runtime::StoreManager &StoreMgr,
                              const uint32_t Idx);
  Expect<void> runGlobalSetOp(Runtime::StoreManager &StoreMgr,
                              const uint32_t Idx);
  /// ======= Table instructions =======
  Expect<void> runTableGetOp(Runtime::Instance::TableInstance &TabInst,
                             const AST::Instruction &Instr);
  Expect<void> runTableSetOp(Runtime::Instance::TableInstance &TabInst,
                             const AST::Instruction &Instr);
  Expect<void> runTableInitOp(Runtime::Instance::TableInstance &TabInst,
                              Runtime::Instance::ElementInstance &ElemInst,
                              const AST::Instruction &Instr);
  Expect<void> runElemDropOp(Runtime::Instance::ElementInstance &ElemInst);
  Expect<void> runTableCopyOp(Runtime::Instance::TableInstance &TabInstDst,
                              Runtime::Instance::TableInstance &TabInstSrc,
                              const AST::Instruction &Instr);
  Expect<void> runTableGrowOp(Runtime::Instance::TableInstance &TabInst);
  Expect<void> runTableSizeOp(Runtime::Instance::TableInstance &TabInst);
  Expect<void> runTableFillOp(Runtime::Instance::TableInstance &TabInst,
                              const AST::Instruction &Instr);
  /// ======= Memory instructions =======
  template <typename T>
  TypeT<T> runLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                     const AST::Instruction &Instr,
                     const uint32_t BitWidth = sizeof(T) * 8);
  template <typename T>
  TypeN<T> runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                      const AST::Instruction &Instr,
                      const uint32_t BitWidth = sizeof(T) * 8);
  Expect<void> runMemorySizeOp(Runtime::Instance::MemoryInstance &MemInst);
  Expect<void> runMemoryGrowOp(Runtime::Instance::MemoryInstance &MemInst);
  Expect<void> runMemoryInitOp(Runtime::Instance::MemoryInstance &MemInst,
                               Runtime::Instance::DataInstance &DataInst,
                               const AST::Instruction &Instr);
  Expect<void> runDataDropOp(Runtime::Instance::DataInstance &DataInst);
  Expect<void> runMemoryCopyOp(Runtime::Instance::MemoryInstance &MemInst,
                               const AST::Instruction &Instr);
  Expect<void> runMemoryFillOp(Runtime::Instance::MemoryInstance &MemInst,
                               const AST::Instruction &Instr);
  /// ======= Test and Relation Numeric instructions =======
  template <typename T> TypeU<T> runEqzOp(ValVariant &Val) const;
  template <typename T>
  TypeT<T> runEqOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeT<T> runNeOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeT<T> runLtOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeT<T> runGtOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeT<T> runLeOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeT<T> runGeOp(ValVariant &Val1, const ValVariant &Val2) const;
  /// ======= Unary Numeric instructions =======
  template <typename T> TypeU<T> runClzOp(ValVariant &Val) const;
  template <typename T> TypeU<T> runCtzOp(ValVariant &Val) const;
  template <typename T> TypeU<T> runPopcntOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runAbsOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runNegOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runCeilOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runFloorOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runTruncOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runNearestOp(ValVariant &Val) const;
  template <typename T> TypeF<T> runSqrtOp(ValVariant &Val) const;
  /// ======= Binary Numeric instructions =======
  template <typename T>
  TypeN<T> runAddOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeN<T> runSubOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeN<T> runMulOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeT<T> runDivOp(const AST::Instruction &Instr, ValVariant &Val1,
                    const ValVariant &Val2) const;
  template <typename T>
  TypeI<T> runRemOp(const AST::Instruction &Instr, ValVariant &Val1,
                    const ValVariant &Val2) const;
  template <typename T>
  TypeU<T> runAndOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeU<T> runOrOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeU<T> runXorOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeU<T> runShlOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeI<T> runShrOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeU<T> runRotlOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeU<T> runRotrOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeF<T> runMinOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeF<T> runMaxOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeF<T> runCopysignOp(ValVariant &Val1, const ValVariant &Val2) const;
  /// ======= Cast Numeric instructions =======
  template <typename TIn, typename TOut>
  TypeUU<TIn, TOut> runWrapOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  TypeFI<TIn, TOut> runTruncateOp(const AST::Instruction &Instr,
                                  ValVariant &Val) const;
  template <typename TIn, typename TOut>
  TypeFI<TIn, TOut> runTruncateSatOp(ValVariant &Val) const;
  template <typename TIn, typename TOut, size_t B = sizeof(TIn) * 8>
  TypeIU<TIn, TOut> runExtendOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  TypeIF<TIn, TOut> runConvertOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  TypeFF<TIn, TOut> runDemoteOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  TypeFF<TIn, TOut> runPromoteOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  TypeNN<TIn, TOut> runReinterpretOp(ValVariant &Val) const;
  /// ======= SIMD Memory instructions =======
  template <typename TIn, typename TOut>
  Expect<void> runLoadExpandOp(Runtime::Instance::MemoryInstance &MemInst,
                               const AST::Instruction &Instr);
  template <typename T>
  Expect<void> runLoadSplatOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr);
  template <typename T>
  Expect<void> runLoadLaneOp(Runtime::Instance::MemoryInstance &MemInst,
                             const AST::Instruction &Instr);
  template <typename T>
  Expect<void> runStoreLaneOp(Runtime::Instance::MemoryInstance &MemInst,
                              const AST::Instruction &Instr);
  /// ======= SIMD Lane instructions =======
  template <typename TIn, typename TOut = TIn>
  Expect<void> runExtractLaneOp(ValVariant &Val, const uint32_t Index) const;
  template <typename TIn, typename TOut = TIn>
  Expect<void> runReplaceLaneOp(ValVariant &Val1, const ValVariant &Val2,
                                const uint32_t Index) const;
  /// ======= SIMD Numeric instructions =======
  template <typename TIn, typename TOut = TIn>
  Expect<void> runSplatOp(ValVariant &Val) const;
  template <typename T>
  Expect<void> runVectorEqOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorNeOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorLtOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorGtOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorLeOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorGeOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T> Expect<void> runVectorAbsOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorNegOp(ValVariant &Val) const;
  inline Expect<void> runVectorPopcntOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorSqrtOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorTruncSatOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorConvertOp(ValVariant &Val) const;
  inline Expect<void> runVectorDemoteOp(ValVariant &Val) const;
  inline Expect<void> runVectorPromoteOp(ValVariant &Val) const;
  inline Expect<void> runVectorAnyTrueOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorAllTrueOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorBitMaskOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorNarrowOp(ValVariant &Val1,
                                 const ValVariant &Val2) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorExtendLowOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorExtendHighOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorExtAddPairwiseOp(ValVariant &Val) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorExtMulLowOp(ValVariant &Val1,
                                    const ValVariant &Val2) const;
  template <typename TIn, typename TOut>
  Expect<void> runVectorExtMulHighOp(ValVariant &Val1,
                                     const ValVariant &Val2) const;
  inline Expect<void> runVectorQ15MulSatOp(ValVariant &Val1,
                                           const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorShlOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorShrOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorAddOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorAddSatOp(ValVariant &Val1,
                                 const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorSubOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorSubSatOp(ValVariant &Val1,
                                 const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorMulOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorDivOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorMinOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorMaxOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorFMinOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  Expect<void> runVectorFMaxOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T, typename ET>
  Expect<void> runVectorAvgrOp(ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T> Expect<void> runVectorCeilOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorFloorOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorTruncOp(ValVariant &Val) const;
  template <typename T> Expect<void> runVectorNearestOp(ValVariant &Val) const;
  /// @}

  /// \name Run compiled functions
  /// @{
public:
  Expect<void> trap(Runtime::StoreManager &StoreMgr,
                    const uint8_t Code) noexcept;
  Expect<void> call(Runtime::StoreManager &StoreMgr, const uint32_t FuncIndex,
                    const ValVariant *Args, ValVariant *Rets) noexcept;
  Expect<void> callIndirect(Runtime::StoreManager &StoreMgr,
                            const uint32_t TableIndex,
                            const uint32_t FuncTypeIndex,
                            const uint32_t FuncIndex, const ValVariant *Args,
                            ValVariant *Rets) noexcept;

  Expect<uint32_t> memGrow(Runtime::StoreManager &StoreMgr,
                           const uint32_t NewSize) noexcept;
  Expect<uint32_t> memSize(Runtime::StoreManager &StoreMgr) noexcept;
  Expect<void> memCopy(Runtime::StoreManager &StoreMgr, const uint32_t Dst,
                       const uint32_t Src, const uint32_t Len) noexcept;
  Expect<void> memFill(Runtime::StoreManager &StoreMgr, const uint32_t Off,
                       const uint8_t Val, const uint32_t Len) noexcept;
  Expect<void> memInit(Runtime::StoreManager &StoreMgr, const uint32_t DataIdx,
                       const uint32_t Dst, const uint32_t Src,
                       const uint32_t Len) noexcept;
  Expect<void> dataDrop(Runtime::StoreManager &StoreMgr,
                        const uint32_t DataIdx) noexcept;

  Expect<RefVariant> tableGet(Runtime::StoreManager &StoreMgr,
                              const uint32_t TableIndex,
                              const uint32_t Idx) noexcept;
  Expect<void> tableSet(Runtime::StoreManager &StoreMgr,
                        const uint32_t TableIndex, const uint32_t Idx,
                        const RefVariant Ref) noexcept;
  Expect<void> tableCopy(Runtime::StoreManager &StoreMgr,
                         const uint32_t TableIndexSrc,
                         const uint32_t TableIndexDst, const uint32_t Dst,
                         const uint32_t Src, const uint32_t Len) noexcept;
  Expect<uint32_t> tableGrow(Runtime::StoreManager &StoreMgr,
                             const uint32_t TableIndex, const RefVariant Val,
                             const uint32_t NewSize) noexcept;
  Expect<uint32_t> tableSize(Runtime::StoreManager &StoreMgr,
                             const uint32_t TableIndex) noexcept;
  Expect<void> tableFill(Runtime::StoreManager &StoreMgr,
                         const uint32_t TableIndex, const uint32_t Off,
                         const RefVariant Ref, const uint32_t Len) noexcept;
  Expect<void> tableInit(Runtime::StoreManager &StoreMgr,
                         const uint32_t TableIndex, const uint32_t ElemIndex,
                         const uint32_t Dst, const uint32_t Src,
                         const uint32_t Len) noexcept;
  Expect<void> elemDrop(Runtime::StoreManager &StoreMgr,
                        const uint32_t ElemIndex) noexcept;
  Expect<RefVariant> refFunc(Runtime::StoreManager &StoreMgr,
                             const uint32_t FuncIndex) noexcept;

  template <typename FuncPtr> struct ProxyHelper;

private:
  /// Pointer to current object.
  static thread_local Interpreter *This;
  /// Store for passing into compiled functions
  Runtime::StoreManager *CurrentStore;
  /// Execution context for compiled functions
  struct ExecutionContext {
    uint8_t *Memory;
    ValVariant *const *Globals;
    uint64_t *InstrCount;
    uint64_t *CostTable;
    uint64_t *Gas;
  } ExecutionContext;
  /// @}

private:
  /// Instantiation mode enumeration class
  enum class InstantiateMode : uint8_t { Instantiate = 0, ImportWasm };
  /// WasmEdge configuration
  const Configure Conf;
  /// Instantiation mode
  InstantiateMode InsMode;
  /// Stack
  Runtime::StackManager StackMgr;
  /// Interpreter statistics
  Statistics::Statistics *Stat;

public:
  /// Callbacks for compiled modules;
  static const AST::Module::IntrinsicsTable Intrinsics;
};

} // namespace Interpreter
} // namespace WasmEdge

#include "engine/binary_numeric.ipp"
#include "engine/cast_numeric.ipp"
#include "engine/memory.ipp"
#include "engine/relation_numeric.ipp"
#include "engine/unary_numeric.ipp"
