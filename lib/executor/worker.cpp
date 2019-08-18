#include "ast/common.h"
#include "ast/instruction.h"
#include "executor/worker.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {

namespace detail {
using OpCode = AST::Instruction::OpCode;
using Value = AST::ValVariant;

/// helper functions for execution
inline bool isInRange(OpCode X, OpCode Y, OpCode Z) {
  auto XC = static_cast<unsigned char>(X);
  auto YC = static_cast<unsigned char>(Y);
  auto ZC = static_cast<unsigned char>(Z);
  return (XC <= YC && YC <= ZC);
}

inline bool isControlOp(OpCode Opcode) {
  return isInRange(OpCode::Unreachable, Opcode, OpCode::Call_indirect);
}

inline bool isParametricOp(OpCode Opcode) {
  return isInRange(OpCode::Drop, Opcode, OpCode::Select);
}

inline bool isVariableOp(OpCode Opcode) {
  return isInRange(OpCode::Local__get, Opcode, OpCode::Global__set);
}

inline bool isLoadOp(OpCode Opcode) {
  return isInRange(OpCode::I32__load, Opcode, OpCode::I64__load32_u);
}

inline bool isStoreOp(OpCode Opcode) {
  return isInRange(OpCode::I32__store, Opcode, OpCode::I64__store32);
}

inline bool isMemoryOp(OpCode Opcode) {
  return isInRange(OpCode::I32__load, Opcode, OpCode::Memory__grow);
}

inline bool isConstNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__const, Opcode, OpCode::F64__const);
}

inline bool isNumericOp(OpCode Opcode) {
  return isInRange(OpCode::I32__eqz, Opcode, OpCode::F64__reinterpret_i64);
}

inline bool isBinaryOp(OpCode Opcode) {
  bool Ret = false;
  switch (Opcode) {
  case OpCode::I32__add:
  case OpCode::I32__sub:
  case OpCode::I64__add:
  case OpCode::I64__sub:
  case OpCode::I64__mul:
  case OpCode::I64__div_u:
  case OpCode::I64__rem_u:
    Ret = true;
    break;
  default:
    Ret = false;
    break;
  }
  return Ret;
}

inline bool isComparisonOp(OpCode Opcode) {
  bool Ret = false;
  switch (Opcode) {
  case OpCode::I32__le_s:
  case OpCode::I32__eq:
  case OpCode::I32__ne:
  case OpCode::I64__eq:
  case OpCode::I64__lt_u:
    Ret = true;
    break;
  default:
    Ret = false;
    break;
  }
  return Ret;
}

} // detail namespace

ErrCode Worker::setArguments(Bytes &Input) {
  Args.assign(Input.begin(), Input.end());
  return ErrCode::Success;
}

ErrCode
Worker::setCode(std::vector<std::unique_ptr<AST::Instruction>> *&Instrs) {
  for (auto &Instr : *Instrs) {
    this->Instrs.push_back(Instr.get());
  }
  return ErrCode::Success;
}

ErrCode Worker::run() {
  ErrCode Status = ErrCode::Success;
  for (auto &Inst : Instrs) {
    OpCode Opcode = Inst->getOpCode();
    if (isConstNumericOp(Opcode)) {
      Status = runConstNumericOp(Inst);
    } else if (isControlOp(Opcode)) {
      Status = runControlOp(Inst);
    } else if (isNumericOp(Opcode)) {
      Status = runNumericOp(Inst);
    } else if (isMemoryOp(Opcode)) {
      Status = runMemoryOp(Inst);
    } else if (isParametricOp(Opcode)) {
      Status = runParametricOp(Inst);
    } else if (isVariableOp(Opcode)) {
      Status = runVariableOp(Inst);
    }

    if (Status != ErrCode::Success) {
      break;
    }
  }

  if (TheState == State::Terminated) {
    return ErrCode::Success;
  } else if (TheState == State::Unreachable) {
    return ErrCode::Unreachable;
  }

  return Status;
}

ErrCode Worker::runConstNumericOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ConstInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  std::unique_ptr<ValueEntry> VE = nullptr;
  std::visit([&VE](auto &&arg) { VE = std::make_unique<ValueEntry>(arg); },
             TheInstrPtr->value());

  StackMgr.push(VE);

  return ErrCode::Success;
}

ErrCode Worker::runNumericOp(AST::Instruction *InstrPtr) {
  /// FIXME: the following calculations do not apply `modulo 2^N`.
  auto TheInstrPtr = dynamic_cast<AST::NumericInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Opcode = TheInstrPtr->getOpCode();
  if (isBinaryOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);
    /// Type check
    AST::ValType ValTp1, ValTp2;
    Val1->getType(ValTp1);
    Val2->getType(ValTp2);
    if (ValTp1 != ValTp2) {
      return ErrCode::TypeNotMatch;
    }

    if (ValTp1 == AST::ValType::I32) {
      int32_t Int1, Int2;
      Val1->getValue(Int1);
      Val2->getValue(Int2);
      if (Opcode == OpCode::I32__add) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 + Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I32__sub) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 - Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else {
        return ErrCode::Unimplemented;
      }
    } else if (ValTp1 == AST::ValType::I64) {
      int64_t Int1, Int2;
      Val1->getValue(Int1);
      Val2->getValue(Int2);
      if (Opcode == OpCode::I64__add) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 + Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I64__sub) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 - Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I64__mul) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 * Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I64__div_u) {
        if (Int2 == 0) {
          return ErrCode::DivideByZero;
        }
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 / Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I64__rem_u) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>(Int1 % Int2);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else {
        return ErrCode::Unimplemented;
      }
    }
    return ErrCode::TypeNotMatch;
  } else if (isComparisonOp(Opcode)) {
    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);
    /// Type check
    AST::ValType ValTp1, ValTp2;
    Val1->getType(ValTp1);
    Val2->getType(ValTp2);
    if (ValTp1 == AST::ValType::I32) {
      int32_t Int1, Int2;
      Val1->getValue(Int1);
      Val2->getValue(Int2);
      if (Opcode == OpCode::I32__le_s) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>((Int1 <= Int2) ? 1 : 0);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I32__eq) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>((Int1 == Int2) ? 1 : 0);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I32__ne) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>((Int1 != Int2) ? 1 : 0);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else {
        return ErrCode::Unimplemented;
      }
    } else if (ValTp1 == AST::ValType::I64) {
      int64_t Int1, Int2;
      Val1->getValue(Int1);
      Val2->getValue(Int2);
      if (Opcode == OpCode::I64__eq) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>((Int1 == Int2) ? 1 : 0);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else if (Opcode == OpCode::I64__lt_u) {
        std::unique_ptr<ValueEntry> NewVal =
            std::make_unique<ValueEntry>((Int1 < Int2) ? 1 : 0);
        StackMgr.push(NewVal);
        return ErrCode::Success;
      } else {
        return ErrCode::Unimplemented;
      }
    }
    return ErrCode::TypeNotMatch;
  } else {
    return ErrCode::Unimplemented;
  }
  return ErrCode::Success;
}

ErrCode Worker::runBrOp(AST::ControlInstruction *Instr) {
  auto BrInstr = dynamic_cast<AST::BrControlInstruction *>(TheInstrPtr);
  if (BrInstr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }
  LabelEntry *Label;
  StackMgr.getLabelWithCount(Label, BrInstr->getLabelIndex());
  unsigned int Arity = Label->getArity();
  std::vector<std::unique_ptr<ValueEntry>> Vals;
  for (int i = 0; i < Arity; i++) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    Vals.push_back(std::move(Val));
  }
  /// Repeat LabelIndex+1 times
  for (int i = 0; i < BrInstr->getLabelIndex()+1; i++) {
    while (StackMgr.isTopValue()) {
      StackMgr.pop();
    }
    /// Pop label entry
    StackMgr.pop();
  }
  /// Push the Vals into the Stack
  for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
    std::unique_ptr<ValueEntry> Val = std::make_unique<ValueEntry>(Iter->release());
    StackMgr.push(Val);
  }
  /// Jump to the continuation of Label
  std::unique_ptr<Worker> NewWorker = std::make_unique<Worker>(StoreMgr, StackMgr);
  NewWorker->setCode(Label->getInstructions());
  Status = NewWorker->run();
  TheState = State::Terminated;
  return Status;
}

ErrCode Worker::runControlOp(AST::Instruction *Instr) {
  auto TheInstrPtr = dynamic_cast<AST::ControlInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  OpCode Opcode = TheInstrPtr->getOpCode();
  if (Opcode == OpCode::Unreachable) {
    TheState = State::Unreachable;
    Status = ErrCode::Unreachable;
  } else if (Opcode == OpCode::Return) {
    StackMgr.getCurrentFrame(CurrentFrame);
    unsigned int Arity = CurrentFrame->getArity();
    std::vector<std::unique_ptr<ValueEntry>> Vals;
    for (int i = 0; i < Arity; i++) {
      std::unique_ptr<ValueEntry> Val;
      StackMgr.pop(Val);
      Vals.push_back(std::move(Val));
    }
    while (StackMgr.isTopFrame()) {
      StackMgr.pop();
    }
    /// Pop the frame entry from the Stack
    StackMgr.pop();
    /// Push the Vals into the Stack
    for (auto Iter = Vals.rbegin(); Iter != Vals.rend(); Iter++) {
      std::unique_ptr<ValueEntry> Val = std::make_unique<ValueEntry>(Iter->release());
      StackMgr.push(Val);
    }
    /// Terminate this worker
    TheState = State::Terminated;
    Status =  ErrCode::Success;
  } else if (Opcode == OpCode::Br) {
    Status = runBrOp(TheInstrPtr);
  } else if (Opcode == OpCode::Br_if) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    int32_t Int32;
    Val.getValue(Int32);
    if (Int32 != 0) {
      Status = runBrOp(TheInstrPtr);
    } else {
      // do nothing
      Status = ErrCode::Success;
    }
  } else {
    Status = ErrCode::Unimplemented;
  }

  return ErrCode::Success;
}

ErrCode Worker::runMemoryOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::MemoryInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  if (isLoadOp(TheInstrPtr->getOpCode())) {
    StackMgr.getCurrentFrame(CurrentFrame);

    unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
    ModuleInstance *ModuleInst = nullptr;
    StoreMgr.getModule(ModuleAddr, ModuleInst);
    unsigned int MemAddr;
    ModuleInst->getMemAddr(0, MemAddr);
    MemoryInstance *MemoryInst = nullptr;
    StoreMgr.getMemory(MemAddr, MemoryInst);

    /// Calculate EA
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);

    int32_t Int;
    Val.getValue(Int);
    int EA = Int + TheInstrPtr->getOffset();
    /// TODO: The following codes do not handle the `t.loadN_sx`.

    /// Get the bit width from the instruction
    OpCode Opcode = TheInstrPtr->getOpCode();
    int N = 0;
    if (Opcode == OpCode::I64__load) {
      N = 64;
    } else if (Opcode == OpCode::I32__load) {
      N = 32;
    } else {
      return ErrCode::Unimplemented;
    }

    /// Make sure the EA + N/8 is NOT larger than length of memory.data.
    if (EA + N/8 > MemoryInst->getDataLength()) {
      return ErrCode::AccessForbidMemory;
    }

    /// Bytes = Mem.Data[EA:N/8]
    std::unique_ptr<std::vector<unsigned char>> BytesPtr = nullptr;
    MemoryInst->getBytes(BytesPtr, EA, N/8);
    std::unique_ptr<ValueEntry> C = nullptr;
    if (Opcode == OpCode::I64__load) {
      /// Construct const C = bytes_64(Bytes);
      C = std::make_unique<ValueEntry>(bytesToInt<int64_t>(*BytesPtr.get()));
    } else if (Opcode == OpCode::I32__load) {
      /// Construct const C = bytes_32(Bytes);
      C = std::make_unique<ValueEntry>(bytesToInt<int32_t>(*BytesPtr.get()));
    } else {
      return ErrCode::Unimplemented;
    }

    /// Push const C to the Stack
    Stack.push(C);
  } else if (isStoreOp(TheInstrPtr->getOpCode())) {
    StackMgr.getCurrentFrame(CurrentFrame);

    unsigned int ModuleAddr = CurrentFrame->getModuleAddr();
    ModuleInstance *ModuleInst = nullptr;
    StoreMgr.getModule(ModuleAddr, ModuleInst);
    unsigned int MemAddr;
    ModuleInst->getMemAddr(0, MemAddr);
    MemoryInstance *MemoryInst = nullptr;
    StoreMgr.getMemory(MemAddr, MemoryInst);

    /// Pop the value t.const c from the Stack
    std::unique_ptr<ValueEntry> C;
    StackMgr.pop(C);
    /// Pop the i32.const i from the Stack
    std::unique_ptr<ValueEntry> I;
    StackMgr.pop(I);
    /// EA = i + offset
    int32_t Int32;
    I.getValue(Int32);
    int32_t EA = Int32 + TheInstrPtr->getOffset();
    /// N = bits(t)
    OpCode Opcode = TheInstrPtr->getOpCode();
    int N = 0;
    if (Opcode == OpCode::I64__store) {
      N = 64;
    } else {
      return ErrCode::Unimplemented;
    }
    /// EA + N/8 <= Memory.Data.Length
    if (EA + N/8 > MemoryInst->getDataLength()) {
      return ErrCode::AccessForbidMemory;
    }

    /// b* = toBytes(c)
    std::vector<unsigned char> Bytes;
    if (Opcode == OpCode::I64__store) {
      int64_t Int64;
      C.getValue(Int64);
      Bytes = IntToBytes<int64_t>(Int64);
    } else {
      return ErrCode::Unimplemented;
    }
    /// Replace the bytes.mem.data[EA:N/8] with b*
    MemoryInst->setBytes(Bytes, EA, N/8);
  } else {
    return ErrCode::Unimplemented;
  }

  return ErrCode::Success;
}

ErrCode Worker::runParametricOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::ParametricInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  if (TheInstrPtr->getOpCode() == OpCode::Drop) {
    StackMgr.pop();
  } else if (TheInstrPtr->getOpCode() == OpCode::Select) {

    // Pop the value i32.const from the stack.
    std::unique_ptr<ValueEntry> VE;
    StackMgr.pop(VE);
    int32_t Val;
    VE->getValue(Val);

    std::unique_ptr<ValueEntry> Val1, Val2;
    StackMgr.pop(Val2);
    StackMgr.pop(Val1);

    if (Val == 0) {
      StackMgr.push(Val2);
    } else {
      StackMgr.push(Val1);
    }
  } else {
    return ErrCode::InstructionTypeMismatch;
  }
  return ErrCode::Success;
}

ErrCode Worker::runVariableOp(AST::Instruction *InstrPtr) {
  auto TheInstrPtr = dynamic_cast<AST::VariableInstruction *>(InstrPtr);
  if (TheInstrPtr == nullptr) {
    return ErrCode::InstructionTypeMismatch;
  }

  auto Opcode = TheInstrPtr->getOpCode();
  unsigned int Index = TheInstrPtr->getIndex();

  if (Opcode == OpCode::Local__get) {
    StackMgr.getCurrentFrame(CurrentFrame);
    ValueEntry *Val;
    CurrentFrame->getValue(Index, Val);
    std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(*Val);
    StackMgr.push(NewVal);
  } else if (Opcode == OpCode::Local__set) {
    StackMgr.getCurrentFrame(CurrentFrame);
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    CurrentFrame->setValue(Index, Val);
  } else if (Opcode == OpCode::Local__tee) {
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    std::unique_ptr<ValueEntry> NewVal =
        std::make_unique<ValueEntry>(*Val.get());
    StackMgr.push(NewVal);
    CurrentFrame->setValue(Index, Val);
  } else if (Opcode == OpCode::Global__get) {
    StackMgr.getCurrentFrame(CurrentFrame);
    ValueEntry Val;
    Instance::GlobalInstance *GlobPtr = nullptr;
    StoreMgr.getGlobal(Index, GlobPtr);
    GlobPtr->getValue(Val);
    std::unique_ptr<ValueEntry> NewVal = std::make_unique<ValueEntry>(Val);
    StackMgr.push(NewVal);
  } else if (Opcode == OpCode::Global__set) {
    StackMgr.getCurrentFrame(CurrentFrame);
    Instance::GlobalInstance *GlobPtr = nullptr;
    StoreMgr.getGlobal(Index, GlobPtr);
    std::unique_ptr<ValueEntry> Val;
    StackMgr.pop(Val);
    GlobPtr->setValue(*Val.get());
  } else {
    return ErrCode::InstructionTypeMismatch;
  }

  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
