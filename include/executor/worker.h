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

  /// Execute const numeric instructions
  ErrCode runConstNumericOp(AST::Instruction *);
  /// Execute numeric instructions
  ErrCode runNumericOp(AST::Instruction *);
  /// Execute coontrol instructions
  ErrCode runControlOp(AST::Instruction *);
  /// Execute memory instructions
  ErrCode runMemoryOp(AST::Instruction *);
  /// Execute parametric instructions
  ErrCode runParametricOp(AST::Instruction *);
  /// Execute variable instructions
  ErrCode runVariableOp(AST::Instruction *);

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
  /// ======= Comparison =======
  template <typename T>
  ErrCode runLeSOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runEqOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runNeOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runLtUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  /// ======= Control =======
  ErrCode runBlockOp(AST::ControlInstruction *Instr);
  ErrCode runLoopOp(AST::ControlInstruction *Instr);
  ErrCode runBrOp(AST::ControlInstruction *Instr);
  ErrCode runBrIfOp(AST::ControlInstruction *Instr);
  ErrCode runReturnOp();
  ErrCode runCallOp(AST::ControlInstruction *Instr);
  /// ======= Memory =======
  template <typename T> ErrCode runLoadOp(AST::MemoryInstruction *InstrPtr);
  template <typename T> ErrCode runStoreOp(AST::MemoryInstruction *InstrPtr);
  /// ======= Numeric =======
  template <typename T>
  ErrCode runAddOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runDivUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runModUOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runMulOp(const ValueEntry *Val1, const ValueEntry *Val2);
  template <typename T>
  ErrCode runSubOp(const ValueEntry *Val1, const ValueEntry *Val2);

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
