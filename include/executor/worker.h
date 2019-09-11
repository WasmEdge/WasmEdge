#pragma once

#include "ast/instruction.h"
#include "common.h"
#include "entry/frame.h"
#include "stackmgr.h"
#include "storemgr.h"
#include "support/casting.h"
#include "worker/provider.h"

#include <cstdint>

namespace SSVM {
namespace Executor {

namespace {

/// Type name aliasing
using Byte = uint8_t;
using Bytes = std::vector<Byte>;
using InstrVec = std::vector<std::unique_ptr<AST::Instruction>>;
using InstrIter = InstrVec::const_iterator;

/// Template return type aliasing
/// Accept unsigned integer types. (uint32_t, uint64_t)
template <typename T, typename TR>
using TypeU = typename std::enable_if_t<Support::IsWasmUnsign<T>::value, TR>;
/// Accept integer types. (uint32_t, int32_t, uint64_t, int64_t)
template <typename T, typename TR>
using TypeI = typename std::enable_if_t<Support::IsWasmInt<T>::value, TR>;
/// Accept floating types. (float, double)
template <typename T, typename TR>
using TypeF = typename std::enable_if_t<Support::IsWasmFloat<T>::value, TR>;
/// Accept all types. (uint32_t, int32_t, uint64_t, int64_t, float, double)
template <typename T, typename TR>
using TypeT = typename std::enable_if_t<Support::IsWasmType<T>::value, TR>;
/// Accept Wasm built-in types. (uint32_t, uint64_t, float, double)
template <typename T, typename TR>
using TypeB = typename std::enable_if_t<Support::IsWasmBuiltIn<T>::value, TR>;

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
  ErrCode runExpression(const InstrVec &Instrs);

  /// Invoke function with main function address.
  ErrCode runStartFunction(unsigned int FuncAddr);

  /// Getter of worker state.
  State getState() const { return TheState; }

private:
  /// Execute Wasm bytecode with given input data.
  ErrCode execute();

  /// Execute coontrol instructions
  ErrCode runControlOp(AST::Instruction *);
  /// Execute parametric instructions
  ErrCode runParametricOp(AST::Instruction *);
  /// Execute variable instructions
  ErrCode runVariableOp(AST::Instruction *);
  /// Execute memory instructions
  ErrCode runMemoryOp(AST::Instruction *);
  /// Execute const numeric instructions
  ErrCode runConstNumericOp(AST::Instruction *);
  /// Execute numeric instructions
  ErrCode runNumericOp(AST::Instruction *);

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
                     const InstrVec &Seq);

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
  template <typename T> ErrCode runLoadOp(AST::MemoryInstruction *InstrPtr);
  template <typename T> ErrCode runStoreOp(AST::MemoryInstruction *InstrPtr);
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
  ErrCode runTAddOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runTSubOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runTMulOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIDivSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIDivUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runFDivOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIRemSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIRemUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIAndOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIOrOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIXorOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIShlOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIShrSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIShrUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIRotlOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runIRotrOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runFMinOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runFMaxOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runFCopysignOp(const ValueEntry *Val1, const ValueEntry *Val2);
  /// ======= Cast Numeric =======
  template <typename TIn, typename TOut>
  ErrCode runWrapOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runTruncSOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runTruncUOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runExtendSOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runExtendUOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runConvertSOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runConvertUOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runDemoteOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runPromoteOp(const ValueEntry *Val);
  template <typename TIn, typename TOut>
  ErrCode runReinterpretOp(const ValueEntry *Val);

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
