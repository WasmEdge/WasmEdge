#pragma once

#include "ast/instruction.h"
#include "common.h"
#include "entry/frame.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "support/casting.h"
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
  explicit Worker(StoreManager &Store, StackManager &Stack)
      : StoreMgr(Store), StackMgr(Stack), TheState(State::Inited){};

  ~Worker() = default;

  /// Prepare input data for calldatacopy.
  ErrCode setArguments(Bytes &Input);

  /// Prepare Wasm bytecode expression for execution.
  ErrCode runExpression(const AST::InstrVec &Instrs);

  /// Invoke function with main function address.
  ErrCode runStartFunction(unsigned int FuncAddr);

  /// Getter of worker state.
  State getState() const { return TheState; }

private:
  /// Execute Wasm bytecode with given input data.
  ErrCode execute();

  /// Execute coontrol instructions
  ErrCode runControlOp(AST::Instruction *Instr);
  /// Execute parametric instructions
  ErrCode runParametricOp(AST::Instruction *Instr);
  /// Execute variable instructions
  ErrCode runVariableOp(AST::Instruction *Instr);
  /// Execute memory instructions
  ErrCode runMemoryOp(AST::Instruction *Instr);
  /// Execute const numeric instructions
  ErrCode runConstNumericOp(AST::Instruction *Instr);
  /// Execute numeric instructions
  ErrCode runNumericOp(AST::Instruction *Instr);

  /// Helper function for entering block control operations.
  ///
  /// Enter into block and push label.
  ///
  /// \param Arity the return counts of this block.
  /// \param Instr the continuous instruction set to label.
  /// \param Seq the entering instruction sequence.
  ///
  /// \returns None.
  ErrCode enterBlock(unsigned int Arity, AST::Instruction *Instr,
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

  /// Helper function for get memory instance by index.
  ErrCode getMemInstByIdx(unsigned int Idx, Instance::MemoryInstance *&MemInst);

  /// Helper function for get global instance by index.
  ErrCode getGlobInstByIdx(unsigned int Idx,
                           Instance::GlobalInstance *&GlobInst);

  /// Run instructions functions
  /// ======= Control =======
  ErrCode runBlockOp(AST::ControlInstruction *Instr);
  ErrCode runLoopOp(AST::ControlInstruction *Instr);
  ErrCode runIfElseOp(AST::ControlInstruction *Instr);
  ErrCode runBrOp(AST::ControlInstruction *Instr);
  ErrCode runBrIfOp(AST::ControlInstruction *Instr);
  ErrCode runBrTableOp(AST::ControlInstruction *Instr);
  ErrCode runReturnOp();
  ErrCode runCallOp(AST::ControlInstruction *Instr);
  ErrCode runCallIndirectOp(AST::ControlInstruction *Instr);
  /// ======= Variable =======
  ErrCode runLocalGetOp(unsigned int Idx);
  ErrCode runLocalSetOp(unsigned int Idx);
  ErrCode runLocalTeeOp(unsigned int Idx);
  ErrCode runGlobalGetOp(unsigned int Idx);
  ErrCode runGlobalSetOp(unsigned int Idx);
  /// ======= Memory =======
  template <typename T>
  TypeT<T, ErrCode> runLoadOp(AST::MemoryInstruction *Instr,
                              unsigned int Width = sizeof(T) * 8);
  template <typename T>
  TypeB<T, ErrCode> runStoreOp(AST::MemoryInstruction *Instr,
                               unsigned int Width = sizeof(T) * 8);
  ErrCode runMemorySizeOp();
  ErrCode runMemoryGrowOp();
  /// ======= Test and Relation Numeric =======
  template <typename T> TypeU<T, ErrCode> runEqzOp(const ValueEntry *Val);
  template <typename T>
  TypeT<T, ErrCode> runEqOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeT<T, ErrCode> runNeOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeT<T, ErrCode> runLtOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeT<T, ErrCode> runGtOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeT<T, ErrCode> runLeOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeT<T, ErrCode> runGeOp(const ValueEntry *Val1, const ValueEntry *Val2);
  /// ======= Unary Numeric =======
  template <typename T> TypeU<T, ErrCode> runClzOp(const ValueEntry *Val);
  template <typename T> TypeU<T, ErrCode> runCtzOp(const ValueEntry *Val);
  template <typename T> TypeU<T, ErrCode> runPopcntOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runAbsOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runNegOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runCeilOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runFloorOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runTruncOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runNearestOp(const ValueEntry *Val);
  template <typename T> TypeF<T, ErrCode> runSqrtOp(const ValueEntry *Val);
  /// ======= Binary Numeric =======
  template <typename T>
  TypeB<T, ErrCode> runAddOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeB<T, ErrCode> runSubOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeB<T, ErrCode> runMulOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeT<T, ErrCode> runDivOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeI<T, ErrCode> runRemOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeU<T, ErrCode> runAndOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeU<T, ErrCode> runOrOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeU<T, ErrCode> runXorOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeU<T, ErrCode> runShlOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeI<T, ErrCode> runShrOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeU<T, ErrCode> runRotlOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeU<T, ErrCode> runRotrOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeF<T, ErrCode> runMinOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeF<T, ErrCode> runMaxOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  TypeF<T, ErrCode> runCopysignOp(const ValueEntry *Val1,
                                  const ValueEntry *Val2);
  /// ======= Cast Numeric =======
  template <typename TIn, typename TOut>
  TypeUU<TIn, TOut, ErrCode> runWrapOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  TypeFI<TIn, TOut, ErrCode> runTruncateOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  TypeIU<TIn, TOut, ErrCode> runExtendOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  TypeIF<TIn, TOut, ErrCode> runConvertOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  TypeFF<TIn, TOut, ErrCode> runDemoteOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  TypeFF<TIn, TOut, ErrCode> runPromoteOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  TypeBB<TIn, TOut, ErrCode> runReinterpretOp(const ValueEntry *Val);

  /// Reference to Executor's Store
  StoreManager &StoreMgr;
  /// Reference to Executor's Stack
  StackManager &StackMgr;
  /// Worker State
  State TheState;
  /// Arguments
  Bytes Args;
  /// Pointer to current frame
  FrameEntry *CurrentFrame;
  /// Instruction provider
  InstrProvider InstrPdr;
};

} // namespace Executor
} // namespace SSVM

#include "worker/binary_numeric.ipp"
#include "worker/cast_numeric.ipp"
#include "worker/memory.ipp"
#include "worker/relation_numeric.ipp"
#include "worker/unary_numeric.ipp"