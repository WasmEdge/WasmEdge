// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ast/instruction.h"
#include "common.h"
#include "entry/frame.h"
#include "hostfuncmgr.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "support/casting.h"
#include "support/time.h"
#include "vm/envmgr.h"
#include "worker/provider.h"

#include <cstdint>
#include <vector>

namespace SSVM {
namespace Executor {

namespace {

/// Type name aliasing
using Byte = uint8_t;
using Bytes = std::vector<Byte>;

/// Template return type aliasing
/// Accept unsigned integer types. (uint32_t, uint64_t)
template <typename T, typename TR>
using TypeU = typename std::enable_if_t<Support::IsWasmUnsignV<T>, TR>;
/// Accept integer types. (uint32_t, int32_t, uint64_t, int64_t)
template <typename T, typename TR>
using TypeI = typename std::enable_if_t<Support::IsWasmIntV<T>, TR>;
/// Accept floating types. (float, double)
template <typename T, typename TR>
using TypeF = typename std::enable_if_t<Support::IsWasmFloatV<T>, TR>;
/// Accept all types. (uint32_t, int32_t, uint64_t, int64_t, float, double)
template <typename T, typename TR>
using TypeT = typename std::enable_if_t<Support::IsWasmTypeV<T>, TR>;
/// Accept Wasm built-in types. (uint32_t, uint64_t, float, double)
template <typename T, typename TR>
using TypeB = typename std::enable_if_t<Support::IsWasmBuiltInV<T>, TR>;

/// Accept (unsigned integer types, unsigned integer types).
template <typename T1, typename T2, typename TR>
using TypeUU = typename std::enable_if_t<
    Support::IsWasmUnsignV<T1> && Support::IsWasmUnsignV<T2>, TR>;
/// Accept (integer types, unsigned integer types).
template <typename T1, typename T2, typename TR>
using TypeIU = typename std::enable_if_t<
    Support::IsWasmIntV<T1> && Support::IsWasmUnsignV<T2>, TR>;
/// Accept (floating types, floating types).
template <typename T1, typename T2, typename TR>
using TypeFF = typename std::enable_if_t<
    Support::IsWasmFloatV<T1> && Support::IsWasmFloatV<T2>, TR>;
/// Accept (integer types, floating types).
template <typename T1, typename T2, typename TR>
using TypeIF = typename std::enable_if_t<
    Support::IsWasmIntV<T1> && Support::IsWasmFloatV<T2>, TR>;
/// Accept (floating types, integer types).
template <typename T1, typename T2, typename TR>
using TypeFI = typename std::enable_if_t<
    Support::IsWasmFloatV<T1> && Support::IsWasmIntV<T2>, TR>;
/// Accept (Wasm built-in types, Wasm built-in types).
template <typename T1, typename T2, typename TR>
using TypeBB = typename std::enable_if_t<Support::IsWasmBuiltInV<T1> &&
                                             Support::IsWasmBuiltInV<T2> &&
                                             sizeof(T1) == sizeof(T2),
                                         TR>;

} // namespace

class Worker {

public:
  enum class State : unsigned char {
    Inited = 0,  /// Default State
    CodeSet,     /// Code set and ready for running
    Active,      /// In execution
    Unreachable, /// Reach `unreachable` instruction
  };

  /// Worker are not allowed to create without Store and Stack.
  Worker() = delete;
  explicit Worker(StoreManager &Store, StackManager &Stack,
                  HostFunctionManager &HostFunc, VM::EnvironmentManager &Env)
      : StoreMgr(Store), StackMgr(Stack), HostFuncMgr(HostFunc), EnvMgr(Env),
        TheState(State::Inited), ExecInstrCnt(0) {}

  /// Prepare Wasm bytecode expression for execution.
  ErrCode runExpression(const AST::InstrVec &Instrs);

  /// Invoke function with main function address.
  ErrCode runStartFunction(unsigned int FuncAddr);

  /// Reset worker.
  ErrCode reset();

  /// Getter of worker state.
  State getState() const { return TheState; }

private:
  friend class AST::ControlInstruction;
  friend class AST::BlockControlInstruction;
  friend class AST::IfElseControlInstruction;
  friend class AST::BrControlInstruction;
  friend class AST::BrTableControlInstruction;
  friend class AST::CallControlInstruction;
  friend class AST::ParametricInstruction;
  friend class AST::VariableInstruction;
  friend class AST::MemoryInstruction;
  friend class AST::ConstInstruction;
  friend class AST::UnaryNumericInstruction;
  friend class AST::BinaryNumericInstruction;
  ErrCode execute(AST::ControlInstruction &);
  ErrCode execute(AST::BlockControlInstruction &);
  ErrCode execute(AST::IfElseControlInstruction &);
  ErrCode execute(AST::BrControlInstruction &);
  ErrCode execute(AST::BrTableControlInstruction &);
  ErrCode execute(AST::CallControlInstruction &);
  ErrCode execute(AST::ParametricInstruction &);
  ErrCode execute(AST::VariableInstruction &);
  ErrCode execute(AST::MemoryInstruction &);
  ErrCode execute(AST::ConstInstruction &);
  ErrCode execute(AST::UnaryNumericInstruction &);
  ErrCode execute(AST::BinaryNumericInstruction &);

private:
  /// Execute Wasm bytecode with given input data.
  ErrCode execute();

  /// Helper function for entering block control operations.
  ///
  /// Enter into block and push label.
  ///
  /// \param Arity the return counts of this block.
  /// \param Instr the continuous instruction set to label.
  /// \param Seq the entering instruction sequence.
  ///
  /// \returns None.
  ErrCode enterBlock(unsigned int Arity, AST::BlockControlInstruction *Instr,
                     const AST::InstrVec &Seq);

  /// Helper function for leaving blocks.
  ErrCode leaveBlock();

  /// Helper function for calling function control operations.
  ///
  /// Enter into function and push frame.
  ///
  /// \param Arity the return counts of this function.
  /// \param Seq the entering instruction sequence.
  ///
  /// \returns None.
  ErrCode invokeFunction(unsigned int FuncAddr);

  /// Helper function for return from functions.
  ErrCode returnFunction();

  /// Helper function for branching to label.
  ErrCode branchToLabel(unsigned int L);

  /// Helper function for get table instance by index.
  ErrCode getTabInstByIdx(unsigned int Idx, Instance::TableInstance *&TabInst);

  /// Helper function for get memory instance by index.
  ErrCode getMemInstByIdx(unsigned int Idx, Instance::MemoryInstance *&MemInst);

  /// Helper function for get global instance by index.
  ErrCode getGlobInstByIdx(unsigned int Idx,
                           Instance::GlobalInstance *&GlobInst);

  /// Run instructions functions
  /// ======= Control =======
  ErrCode runBlockOp(AST::BlockControlInstruction &Instr);
  ErrCode runLoopOp(AST::BlockControlInstruction &Instr);
  ErrCode runIfElseOp(AST::IfElseControlInstruction &Instr);
  ErrCode runBrOp(AST::BrControlInstruction &Instr);
  ErrCode runBrIfOp(AST::BrControlInstruction &Instr);
  ErrCode runBrTableOp(AST::BrTableControlInstruction &Instr);
  ErrCode runReturnOp();
  ErrCode runCallOp(AST::CallControlInstruction &Instr);
  ErrCode runCallIndirectOp(AST::CallControlInstruction &Instr);
  /// ======= Variable =======
  ErrCode runLocalGetOp(unsigned int Idx);
  ErrCode runLocalSetOp(unsigned int Idx);
  ErrCode runLocalTeeOp(unsigned int Idx);
  ErrCode runGlobalGetOp(unsigned int Idx);
  ErrCode runGlobalSetOp(unsigned int Idx);
  /// ======= Memory =======
  template <typename T>
  TypeT<T, ErrCode> runLoadOp(AST::MemoryInstruction &Instr,
                              unsigned int BitWidth = sizeof(T) * 8);
  template <typename T>
  TypeB<T, ErrCode> runStoreOp(AST::MemoryInstruction &Instr,
                               unsigned int BitWidth = sizeof(T) * 8);
  ErrCode runMemorySizeOp();
  ErrCode runMemoryGrowOp();
  /// ======= Test and Relation Numeric =======
  template <typename T> TypeU<T, ErrCode> runEqzOp(Value &Val) const;
  template <typename T>
  TypeT<T, ErrCode> runEqOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeT<T, ErrCode> runNeOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeT<T, ErrCode> runLtOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeT<T, ErrCode> runGtOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeT<T, ErrCode> runLeOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeT<T, ErrCode> runGeOp(Value &Val1, const Value &Val2) const;
  /// ======= Unary Numeric =======
  template <typename T> TypeU<T, ErrCode> runClzOp(Value &Val) const;
  template <typename T> TypeU<T, ErrCode> runCtzOp(Value &Val) const;
  template <typename T> TypeU<T, ErrCode> runPopcntOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runAbsOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runNegOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runCeilOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runFloorOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runTruncOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runNearestOp(Value &Val) const;
  template <typename T> TypeF<T, ErrCode> runSqrtOp(Value &Val) const;
  /// ======= Binary Numeric =======
  template <typename T>
  TypeB<T, ErrCode> runAddOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeB<T, ErrCode> runSubOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeB<T, ErrCode> runMulOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeT<T, ErrCode> runDivOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeI<T, ErrCode> runRemOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeU<T, ErrCode> runAndOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeU<T, ErrCode> runOrOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeU<T, ErrCode> runXorOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeU<T, ErrCode> runShlOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeI<T, ErrCode> runShrOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeU<T, ErrCode> runRotlOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeU<T, ErrCode> runRotrOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeF<T, ErrCode> runMinOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeF<T, ErrCode> runMaxOp(Value &Val1, const Value &Val2) const;
  template <typename T>
  TypeF<T, ErrCode> runCopysignOp(Value &Val1, const Value &Val2) const;
  /// ======= Cast Numeric =======
  template <typename TIn, typename TOut>
  TypeUU<TIn, TOut, ErrCode> runWrapOp(Value &Val) const;
  template <typename TIn, typename TOut>
  TypeFI<TIn, TOut, ErrCode> runTruncateOp(Value &Val) const;
  template <typename TIn, typename TOut>
  TypeIU<TIn, TOut, ErrCode> runExtendOp(Value &Val) const;
  template <typename TIn, typename TOut>
  TypeIF<TIn, TOut, ErrCode> runConvertOp(Value &Val) const;
  template <typename TIn, typename TOut>
  TypeFF<TIn, TOut, ErrCode> runDemoteOp(Value &Val) const;
  template <typename TIn, typename TOut>
  TypeFF<TIn, TOut, ErrCode> runPromoteOp(Value &Val) const;
  template <typename TIn, typename TOut>
  TypeBB<TIn, TOut, ErrCode> runReinterpretOp(Value &Val) const;

  /// Reference to Executor's Store
  StoreManager &StoreMgr;
  /// Reference to Executor's Stack
  StackManager &StackMgr;
  /// Reference to Executor's Host function manager
  HostFunctionManager &HostFuncMgr;
  /// Reference to Environment manager
  VM::EnvironmentManager &EnvMgr;
  /// Worker State
  State TheState;
  /// Instruction provider
  InstrProvider InstrPdr;

  /// Time recorder
  Support::TimeRecord TimeRecorder;
  /// Instruction Counts
  uint64_t ExecInstrCnt;
};

} // namespace Executor
} // namespace SSVM

#include "worker/binary_numeric.ipp"
#include "worker/cast_numeric.ipp"
#include "worker/memory.ipp"
#include "worker/relation_numeric.ipp"
#include "worker/unary_numeric.ipp"
