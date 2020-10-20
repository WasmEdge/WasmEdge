// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/interpreter/interpreter.h - Interpreter class definition -----===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Interpreter class, which
/// instantiate and run Wasm modules.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/ast/instruction.h"
#include "common/ast/module.h"
#include "common/errcode.h"
#include "common/statistics.h"
#include "common/value.h"
#include "runtime/importobj.h"
#include "runtime/stackmgr.h"
#include "runtime/storemgr.h"
#include "support/measure.h"
#include "support/time.h"

#include <cassert>
#include <csetjmp>
#include <csignal>
#include <memory>
#include <type_traits>
#include <vector>

namespace SSVM {
namespace Interpreter {

namespace {

/// Template return type aliasing
/// Accept unsigned integer types. (uint32_t, uint64_t)
template <typename T>
using TypeU =
    typename std::enable_if_t<Support::IsWasmUnsignV<T>, Expect<void>>;
/// Accept integer types. (uint32_t, int32_t, uint64_t, int64_t)
template <typename T>
using TypeI = typename std::enable_if_t<Support::IsWasmIntV<T>, Expect<void>>;
/// Accept floating types. (float, double)
template <typename T>
using TypeF = typename std::enable_if_t<Support::IsWasmFloatV<T>, Expect<void>>;
/// Accept all num types. (uint32_t, int32_t, uint64_t, int64_t, float, double)
template <typename T>
using TypeT = typename std::enable_if_t<Support::IsWasmNumV<T>, Expect<void>>;
/// Accept Wasm built-in num types. (uint32_t, uint64_t, float, double)
template <typename T>
using TypeN =
    typename std::enable_if_t<Support::IsWasmNativeNumV<T>, Expect<void>>;

/// Accept (unsigned integer types, unsigned integer types).
template <typename T1, typename T2>
using TypeUU = typename std::enable_if_t<
    Support::IsWasmUnsignV<T1> && Support::IsWasmUnsignV<T2>, Expect<void>>;
/// Accept (integer types, unsigned integer types).
template <typename T1, typename T2>
using TypeIU = typename std::enable_if_t<
    Support::IsWasmIntV<T1> && Support::IsWasmUnsignV<T2>, Expect<void>>;
/// Accept (floating types, floating types).
template <typename T1, typename T2>
using TypeFF = typename std::enable_if_t<
    Support::IsWasmFloatV<T1> && Support::IsWasmFloatV<T2>, Expect<void>>;
/// Accept (integer types, floating types).
template <typename T1, typename T2>
using TypeIF = typename std::enable_if_t<
    Support::IsWasmIntV<T1> && Support::IsWasmFloatV<T2>, Expect<void>>;
/// Accept (floating types, integer types).
template <typename T1, typename T2>
using TypeFI = typename std::enable_if_t<
    Support::IsWasmFloatV<T1> && Support::IsWasmIntV<T2>, Expect<void>>;
/// Accept (Wasm built-in num types, Wasm built-in num types).
template <typename T1, typename T2>
using TypeNN = typename std::enable_if_t<Support::IsWasmNativeNumV<T1> &&
                                             Support::IsWasmNativeNumV<T2> &&
                                             sizeof(T1) == sizeof(T2),
                                         Expect<void>>;

} // namespace

/// Executor flow control class.
class Interpreter {
public:
  Interpreter(Support::Measurement *M = nullptr,
              Statistics::Statistics *S = nullptr)
      : Measure(M), Stat(S) {
    assert(This == nullptr);
    This = this;
  }
  ~Interpreter() noexcept { This = nullptr; }

  /// Instantiate Wasm Module.
  Expect<void> instantiateModule(Runtime::StoreManager &StoreMgr,
                                 const AST::Module &Mod,
                                 std::string_view Name = {});

  /// Register host module.
  Expect<void> registerModule(Runtime::StoreManager &StoreMgr,
                              const Runtime::ImportObject &Obj);

  /// Register Wasm module.
  Expect<void> registerModule(Runtime::StoreManager &StoreMgr,
                              const AST::Module &Mod, std::string_view Name);

  /// Invoke function by function address in Store manager.
  Expect<std::vector<ValVariant>> invoke(Runtime::StoreManager &StoreMgr,
                                         const uint32_t FuncAddr,
                                         Span<const ValVariant> Params);

private:
  /// Run Wasm bytecode expression for initialization.
  Expect<void> runExpression(Runtime::StoreManager &StoreMgr,
                             const AST::InstrVec &Instrs);

  /// Run Wasm function.
  Expect<void> runFunction(Runtime::StoreManager &StoreMgr,
                           const Runtime::Instance::FunctionInstance &Func,
                           Span<const ValVariant> Params);

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

  /// Instantiation of Data Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::DataSection &DataSec);

  /// Instantiation of Export Instances.
  Expect<void> instantiate(Runtime::StoreManager &StoreMgr,
                           Runtime::Instance::ModuleInstance &ModInst,
                           const AST::ExportSection &ExportSec);
  /// @}

  /// \name Functions for instruction dispatchers.
  /// @{
  Expect<void> execute(Runtime::StoreManager &StoreMgr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::ControlInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::BlockControlInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::IfElseControlInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::BrControlInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::BrTableControlInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::CallControlInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::ReferenceInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::ParametricInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::VariableInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::TableInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::MemoryInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::ConstInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::UnaryNumericInstruction &Instr);
  Expect<void> execute(Runtime::StoreManager &StoreMgr,
                       const AST::BinaryNumericInstruction &Instr);
  /// @}

  /// \name Helper Functions for block controls.
  /// @{
  /// Helper function for entering blocks.
  Expect<void> enterBlock(const uint32_t Locals, const uint32_t Arity,
                          const AST::BlockControlInstruction *Instr,
                          const AST::InstrVec &Seq);

  /// Helper function for calling functions.
  Expect<void> enterFunction(Runtime::StoreManager &StoreMgr,
                             const Runtime::Instance::FunctionInstance &Func);

  /// Helper function for branching to label.
  Expect<void> branchToLabel(Runtime::StoreManager &StoreMgr,
                             const uint32_t Cnt);
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
                          const AST::BlockControlInstruction &Instr);
  Expect<void> runLoopOp(Runtime::StoreManager &StoreMgr,
                         const AST::BlockControlInstruction &Instr);
  Expect<void> runIfElseOp(Runtime::StoreManager &StoreMgr,
                           const AST::IfElseControlInstruction &Instr);
  Expect<void> runBrOp(Runtime::StoreManager &StoreMgr,
                       const AST::BrControlInstruction &Instr);
  Expect<void> runBrIfOp(Runtime::StoreManager &StoreMgr,
                         const AST::BrControlInstruction &Instr);
  Expect<void> runBrTableOp(Runtime::StoreManager &StoreMgr,
                            const AST::BrTableControlInstruction &Instr);
  Expect<void> runReturnOp();
  Expect<void> runCallOp(Runtime::StoreManager &StoreMgr,
                         const AST::CallControlInstruction &Instr);
  Expect<void> runCallIndirectOp(Runtime::StoreManager &StoreMgr,
                                 const AST::CallControlInstruction &Instr);
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
                             const AST::TableInstruction &Instr);
  Expect<void> runTableSetOp(Runtime::Instance::TableInstance &TabInst,
                             const AST::TableInstruction &Instr);
  Expect<void> runTableInitOp(Runtime::Instance::TableInstance &TabInst,
                              Runtime::Instance::ElementInstance &ElemInst,
                              const AST::TableInstruction &Instr);
  Expect<void> runElemDropOp(Runtime::Instance::ElementInstance &ElemInst);
  Expect<void> runTableCopyOp(Runtime::Instance::TableInstance &TabInstDst,
                              Runtime::Instance::TableInstance &TabInstSrc,
                              const AST::TableInstruction &Instr);
  Expect<void> runTableGrowOp(Runtime::Instance::TableInstance &TabInst);
  Expect<void> runTableSizeOp(Runtime::Instance::TableInstance &TabInst);
  Expect<void> runTableFillOp(Runtime::Instance::TableInstance &TabInst,
                              const AST::TableInstruction &Instr);
  /// ======= Memory instructions =======
  template <typename T>
  TypeT<T> runLoadOp(Runtime::Instance::MemoryInstance &MemInst,
                     const AST::MemoryInstruction &Instr,
                     const uint32_t BitWidth = sizeof(T) * 8);
  template <typename T>
  TypeN<T> runStoreOp(Runtime::Instance::MemoryInstance &MemInst,
                      const AST::MemoryInstruction &Instr,
                      const uint32_t BitWidth = sizeof(T) * 8);
  Expect<void> runMemorySizeOp(Runtime::Instance::MemoryInstance &MemInst);
  Expect<void> runMemoryGrowOp(Runtime::Instance::MemoryInstance &MemInst);
  Expect<void> runMemoryInitOp(Runtime::Instance::MemoryInstance &MemInst,
                               Runtime::Instance::DataInstance &DataInst,
                               const AST::MemoryInstruction &Instr);
  Expect<void> runDataDropOp(Runtime::Instance::DataInstance &DataInst);
  Expect<void> runMemoryCopyOp(Runtime::Instance::MemoryInstance &MemInst,
                               const AST::MemoryInstruction &Instr);
  Expect<void> runMemoryFillOp(Runtime::Instance::MemoryInstance &MemInst,
                               const AST::MemoryInstruction &Instr);
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
  TypeT<T> runDivOp(const AST::BinaryNumericInstruction &Instr,
                    ValVariant &Val1, const ValVariant &Val2) const;
  template <typename T>
  TypeI<T> runRemOp(const AST::BinaryNumericInstruction &Instr,
                    ValVariant &Val1, const ValVariant &Val2) const;
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
  TypeFI<TIn, TOut> runTruncateOp(const AST::UnaryNumericInstruction &Instr,
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
  /// @}

  /// \name Run compiled functions
  /// @{

  /// Pointer to current object.
  static Interpreter *This;
  /// jmp_buf for trap.
  static sigjmp_buf *TrapJump;
  static uint32_t TrapCode;
  static AST::Module::IntrinsicsTable IntrinsicsTable;

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

  Expect<ValVariant> tableGet(Runtime::StoreManager &StoreMgr,
                              const uint32_t TableIndex,
                              const uint32_t Idx) noexcept;
  Expect<void> tableSet(Runtime::StoreManager &StoreMgr,
                        const uint32_t TableIndex, const uint32_t Idx,
                        const ValVariant Ref) noexcept;
  Expect<void> tableCopy(Runtime::StoreManager &StoreMgr,
                         const uint32_t TableIndexSrc,
                         const uint32_t TableIndexDst, const uint32_t Dst,
                         const uint32_t Src, const uint32_t Len) noexcept;
  Expect<uint32_t> tableGrow(Runtime::StoreManager &StoreMgr,
                             const uint32_t TableIndex, const ValVariant Val,
                             const uint32_t NewSize) noexcept;
  Expect<uint32_t> tableSize(Runtime::StoreManager &StoreMgr,
                             const uint32_t TableIndex) noexcept;
  Expect<void> tableFill(Runtime::StoreManager &StoreMgr,
                         const uint32_t TableIndex, const uint32_t Off,
                         const ValVariant Ref, const uint32_t Len) noexcept;
  Expect<void> tableInit(Runtime::StoreManager &StoreMgr,
                         const uint32_t TableIndex, const uint32_t ElemIndex,
                         const uint32_t Dst, const uint32_t Src,
                         const uint32_t Len) noexcept;
  Expect<void> elemDrop(Runtime::StoreManager &StoreMgr,
                        const uint32_t ElemIndex) noexcept;
  Expect<ValVariant> refFunc(Runtime::StoreManager &StoreMgr,
                             const uint32_t FuncIndex) noexcept;

  static void signalEnable() noexcept;
  static void signalDisable() noexcept;
  static void signalHandler(int Signal, siginfo_t *Siginfo, void *) noexcept;
  struct SignalEnabler;
  struct SignalDisabler;
  template <typename FuncPtr> struct ProxyHelper;
  /// @}

  enum class InstantiateMode : uint8_t { Instantiate = 0, ImportWasm };

  /// Instantiate mode
  InstantiateMode InsMode;
  /// Stack
  Runtime::StackManager StackMgr;
  /// Pointer to measurement.
  Support::Measurement *Measure;
  /// Interpreter statistics
  Statistics::Statistics *Stat;
  Runtime::StoreManager *CurrentStore;
};

} // namespace Interpreter
} // namespace SSVM

#include "engine/binary_numeric.ipp"
#include "engine/cast_numeric.ipp"
#include "engine/memory.ipp"
#include "engine/relation_numeric.ipp"
#include "engine/unary_numeric.ipp"
