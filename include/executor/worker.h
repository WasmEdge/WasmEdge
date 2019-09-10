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

class Worker {

public:
  using Byte = uint8_t;
  using Bytes = std::vector<Byte>;
  using InstrVec = std::vector<std::unique_ptr<AST::Instruction>>;
  using InstrIter = InstrVec::const_iterator;

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
  template <typename T> ErrCode runEqzOp(const ValueEntry *Val);
  template <typename T>
  ErrCode runEqIOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runEqFOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runNeIOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runNeFOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLtSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLtUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLtFOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runGtSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runGtUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runGtFOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLeSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLeUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLeFOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runGeSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runGeUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runGeFOp(const ValueEntry *Val1, const ValueEntry *Val2);
  /// ======= Unary Numeric =======
  template <typename T> ErrCode runClzOp(const ValueEntry *Val);
  template <typename T> ErrCode runCtzOp(const ValueEntry *Val);
  template <typename T> ErrCode runPopcntOp(const ValueEntry *Val);
  template <typename T> ErrCode runAbsOp(const ValueEntry *Val);
  template <typename T> ErrCode runNegOp(const ValueEntry *Val);
  template <typename T> ErrCode runCeilOp(const ValueEntry *Val);
  template <typename T> ErrCode runFloorOp(const ValueEntry *Val);
  template <typename T> ErrCode runTruncOp(const ValueEntry *Val);
  template <typename T> ErrCode runNearestOp(const ValueEntry *Val);
  template <typename T> ErrCode runSqrtOp(const ValueEntry *Val);
  /// ======= Binary Numeric =======
  template <typename T>
  ErrCode runAddOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runSubOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runMulOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runDivSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runDivUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runRemSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runRemUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runMinOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runMaxOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runCopysignOp(const ValueEntry *Val1, const ValueEntry *Val2);
  /// ======= Casting Numeric =======
  template <typename T> ErrCode runWrapOp(const ValueEntry *Val);
  template <typename T> ErrCode runTruncSOp(const ValueEntry *Val);
  template <typename T> ErrCode runTruncUOp(const ValueEntry *Val);
  template <typename T> ErrCode runExtendSOp(const ValueEntry *Val);
  template <typename T> ErrCode runExtendUOp(const ValueEntry *Val);
  template <typename T> ErrCode runConvertSOp(const ValueEntry *Val);
  template <typename T> ErrCode runConvertUOp(const ValueEntry *Val);
  template <typename T> ErrCode runDemoteOp(const ValueEntry *Val);
  template <typename T> ErrCode runPromoteOp(const ValueEntry *Val);
  template <typename T> ErrCode runReinterpretOp(const ValueEntry *Val);

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
