// SPDX-License-Identifier: Apache-2.0
#include "ast/instruction.h"
#include "common/log.h"

namespace SSVM {
namespace AST {

/// Copy construtor. See "include/ast/instruction.h".
BlockControlInstruction::BlockControlInstruction(
    const BlockControlInstruction &Instr)
    : Instruction(Instr.Code, Instr.Offset), ResType(Instr.ResType) {
  for (auto &It : Instr.Body) {
    if (auto Res = makeInstructionNode(*It.get())) {
      Body.push_back(std::move(*Res));
    }
  }
}

/// Load binary of block instructions. See "include/ast/instruction.h".
Expect<void> BlockControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the block return type.
  if (auto Res = Mgr.readS32()) {
    if (*Res < 0) {
      /// Value type case.
      ValType VType = static_cast<ValType>((*Res) & 0x7FU);
      switch (VType) {
      case ValType::None:
      case ValType::I32:
      case ValType::I64:
      case ValType::F32:
      case ValType::F64:
      case ValType::V128:
      case ValType::ExternRef:
      case ValType::FuncRef:
        ResType = VType;
        break;
      default:
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      /// Type index case.
      ResType = static_cast<uint32_t>(*Res);
    }
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }

  if (auto Res = loadInstrSeq(Mgr)) {
    Body = std::move(*Res);
  } else {
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  return {};
}

/// Copy construtor. See "include/ast/instruction.h".
IfElseControlInstruction::IfElseControlInstruction(
    const IfElseControlInstruction &Instr)
    : Instruction(Instr.Code, Instr.Offset), ResType(Instr.ResType) {
  for (auto &It : Instr.IfStatement) {
    if (auto Res = makeInstructionNode(*It.get())) {
      IfStatement.push_back(std::move(*Res));
    }
  }
  for (auto &It : Instr.ElseStatement) {
    if (auto Res = makeInstructionNode(*It.get())) {
      ElseStatement.push_back(std::move(*Res));
    }
  }
}

/// Load binary of if-else instructions. See "include/ast/instruction.h".
Expect<void> IfElseControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the block return type.
  if (auto Res = Mgr.readS32()) {
    if (*Res < 0) {
      /// Value type case.
      ValType VType = static_cast<ValType>((*Res) & 0x7FU);
      switch (VType) {
      case ValType::None:
      case ValType::I32:
      case ValType::I64:
      case ValType::F32:
      case ValType::F64:
      case ValType::V128:
      case ValType::ExternRef:
      case ValType::FuncRef:
        ResType = VType;
        break;
      default:
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      /// Type index case.
      ResType = static_cast<uint32_t>(*Res);
    }
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }

  /// Read instructions and make nodes until OpCode::End.
  ssize_t ElsePos = -1;
  if (auto Res = loadInstrSeq(Mgr, &ElsePos)) {
    IfStatement = std::move(*Res);
  } else {
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  if (ElsePos != -1) {
    ElseStatement.insert(ElseStatement.end(),
                         std::make_move_iterator(IfStatement.begin() + ElsePos),
                         std::make_move_iterator(IfStatement.end()));
    IfStatement.erase(IfStatement.begin() + ElsePos, IfStatement.end());
    IfStatement.push_back(std::make_unique<ControlInstruction>(
        OpCode::End, ElseStatement.back()->getOffset()));
  } else {
    ElseStatement.push_back(std::make_unique<ControlInstruction>(
        OpCode::End, IfStatement.back()->getOffset()));
  }
  return {};
}

/// Load binary of branch instructions. See "include/ast/instruction.h".
Expect<void> BrControlInstruction::loadBinary(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    LabelIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  return {};
}

/// Load branch table instructions. See "include/ast/instruction.h".
Expect<void> BrTableControlInstruction::loadBinary(FileMgr &Mgr) {
  uint32_t VecCnt = 0;

  /// Read the vector of labels.
  if (auto Res = Mgr.readU32()) {
    VecCnt = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  for (uint32_t i = 0; i < VecCnt; ++i) {
    if (auto Res = Mgr.readU32()) {
      LabelList.push_back(*Res);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
  }

  /// Read default label.
  if (auto Res = Mgr.readU32()) {
    LabelIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  return {};
}

/// Load binary of call instructions. See "include/ast/instruction.h".
Expect<void> CallControlInstruction::loadBinary(FileMgr &Mgr) {
  /// Read function index.
  if (auto Res = Mgr.readU32()) {
    TargetIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }

  /// Read the table index in indirect_call case.
  if (Code == OpCode::Call_indirect) {
    if (auto Res = Mgr.readU32()) {
      TableIdx = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
  }

  return {};
}

/// Load reference instructions. See "include/ast/instruction.h".
Expect<void> ReferenceInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the reftype and funcidx.
  switch (Code) {
  case OpCode::Ref__null:
    if (auto Res = Mgr.readByte()) {
      Type = static_cast<RefType>(*Res);
      switch (Type) {
      case RefType::FuncRef:
      case RefType::ExternRef:
        break;
      default:
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;

  case OpCode::Ref__func:
    if (auto Res = Mgr.readU32()) {
      TargetIdx = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;

  case OpCode::Ref__is_null:
  default:
    break;
  }
  return {};
}

/// Load variable instructions. See "include/ast/instruction.h".
Expect<void> ParametricInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the valtype vector in select (t*) case.
  if (Code == OpCode::Select_t) {
    uint32_t VecCnt = 0;
    if (auto Res = Mgr.readU32()) {
      VecCnt = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    for (uint32_t i = 0; i < VecCnt; ++i) {
      if (auto Res = Mgr.readByte()) {
        ValType VType = static_cast<ValType>(*Res);
        switch (VType) {
        case ValType::None:
        case ValType::I32:
        case ValType::I64:
        case ValType::F32:
        case ValType::F64:
        case ValType::V128:
        case ValType::ExternRef:
        case ValType::FuncRef:
          ValTypeList.push_back(VType);
          break;
        default:
          LOG(ERROR) << ErrCode::InvalidGrammar;
          LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
          LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
          return Unexpect(ErrCode::InvalidGrammar);
        }
      } else {
        LOG(ERROR) << Res.error();
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(Res);
      }
    }
  }
  return {};
}

/// Load variable instructions. See "include/ast/instruction.h".
Expect<void> VariableInstruction::loadBinary(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    VarIdx = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  return {};
}

/// Load binary of table instructions. See "include/ast/instruction.h".
Expect<void> TableInstruction::loadBinary(FileMgr &Mgr) {
  if (auto Res = Mgr.readU32()) {
    switch (Code) {
    case OpCode::Table__get:
    case OpCode::Table__set:
    case OpCode::Table__grow:
    case OpCode::Table__size:
    case OpCode::Table__fill:
      TargetIdx = *Res;
      return {};

    case OpCode::Elem__drop:
      ElemIdx = *Res;
      return {};

    case OpCode::Table__copy:
      TargetIdx = *Res;
      break;
    case OpCode::Table__init:
      ElemIdx = *Res;
      break;
    default:
      break;
    }
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }

  /// Read second index in table.init and table.copy case.
  if (auto Res = Mgr.readU32()) {
    switch (Code) {
    case OpCode::Table__copy:
      SourceIdx = *Res;
      break;
    case OpCode::Table__init:
      TargetIdx = *Res;
      break;
    default:
      break;
    }
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  return {};
}

/// Load binary of memory instructions. See "include/ast/instruction.h".
Expect<void> MemoryInstruction::loadBinary(FileMgr &Mgr) {
  auto readCheck = [&Mgr]() -> Expect<void> {
    if (auto Res = Mgr.readByte()) {
      if (*Res != 0x00) {
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(ErrCode::InvalidGrammar);
      }
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    return {};
  };

  switch (Code) {
  /// Read the 0x00 checking code in memory.grow, memory.size, memory.fill, and
  /// memory.copy cases.
  case OpCode::Memory__copy:
    if (auto Res = readCheck(); !Res) {
      return Unexpect(Res);
    }
    [[fallthrough]];
  case OpCode::Memory__grow:
  case OpCode::Memory__size:
  case OpCode::Memory__fill:
    if (auto Res = readCheck(); !Res) {
      return Unexpect(Res);
    }
    break;

  /// Read data index in memory.init and data.drop cases.
  case OpCode::Memory__init:
  case OpCode::Data__drop:
    if (auto Res = Mgr.readU32()) {
      DataIdx = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    if (Code == OpCode::Memory__init) {
      /// memory.init has 0x00 code.
      if (auto Res = readCheck(); !Res) {
        return Unexpect(Res);
      }
    }
    break;

  default:
    /// Read memory arguments.
    if (auto Res = Mgr.readU32()) {
      Align = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    if (auto Res = Mgr.readU32()) {
      Offset = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;
  }
  return {};
}

/// Load const numeric instructions. See "include/ast/instruction.h".
Expect<void> ConstInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the const number of corresbonding number type.
  switch (Code) {
  case OpCode::I32__const:
    if (auto Res = Mgr.readS32()) {
      Num = static_cast<uint32_t>(*Res);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;
  case OpCode::I64__const:
    if (auto Res = Mgr.readS64()) {
      Num = static_cast<uint64_t>(*Res);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;
  case OpCode::F32__const:
    if (auto Res = Mgr.readF32()) {
      Num = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;
  case OpCode::F64__const:
    if (auto Res = Mgr.readF64()) {
      Num = *Res;
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    break;
  default:
    LOG(ERROR) << ErrCode::InvalidGrammar;
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset() - 1);
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(ErrCode::InvalidGrammar);
  }

  return {};
}

/// Load SIMD memory instructions. See "include/ast/instruction.h".
Expect<void> SIMDMemoryInstruction::loadBinary(FileMgr &Mgr) {
  /// Read memory arguments.
  if (auto Res = Mgr.readU32()) {
    Align = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }
  if (auto Res = Mgr.readU32()) {
    Offset = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }

  return {};
}

/// Load SIMD const instructions. See "include/ast/instruction.h".
Expect<void> SIMDConstInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the constant number in little endian.
  uint128_t Value = 0;
  for (uint32_t I = 0; I < 16; ++I) {
    if (auto Res = Mgr.readByte()) {
      Value |= uint128_t(*Res) << (I * 8);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
  }
  Num = Value;

  return {};
}

/// Load SIMD shuffle instructions. See "include/ast/instruction.h".
Expect<void> SIMDShuffleInstruction::loadBinary(FileMgr &Mgr) {
  /// Read lane indexes.
  uint128_t Value = 0;
  for (uint32_t I = 0; I < 16; ++I) {
    if (auto Res = Mgr.readByte()) {
      Value |= uint128_t(*Res) << (I * 8);
    } else {
      LOG(ERROR) << Res.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
  }
  Num = Value;

  return {};
}

/// Load SIMD lane instructions. See "include/ast/instruction.h".
Expect<void> SIMDLaneInstruction::loadBinary(FileMgr &Mgr) {
  /// Read lane index.
  if (auto Res = Mgr.readByte()) {
    Index = *Res;
  } else {
    LOG(ERROR) << Res.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
    return Unexpect(Res);
  }

  return {};
}

/// OpCode loader. See "include/ast/instruction.h".
Expect<OpCode> loadOpCode(FileMgr &Mgr) {
  uint16_t Payload;
  if (auto B1 = Mgr.readByte()) {
    Payload = (*B1);
  } else {
    LOG(ERROR) << B1.error();
    LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
    return Unexpect(B1);
  }

  if (Payload == 0xFCU) {
    /// 2-bytes OpCode case.
    if (auto B2 = Mgr.readByte()) {
      Payload <<= 8;
      Payload |= (*B2);
    } else {
      LOG(ERROR) << B2.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      return Unexpect(B2);
    }
  } else if (Payload == 0xFDU) {
    /// 2-bytes OpCode case.
    if (auto B2 = Mgr.readU32()) {
      Payload <<= 8;
      Payload += (*B2);
    } else {
      LOG(ERROR) << B2.error();
      LOG(ERROR) << ErrInfo::InfoLoading(Mgr.getOffset());
      return Unexpect(B2);
    }
  }
  return static_cast<OpCode>(Payload);
}

Expect<InstrVec> loadInstrSeq(FileMgr &Mgr, ssize_t *MeasureElseOp) {
  InstrVec Instrs;
  bool IsElseOpOccurred = false;

  /// Read opcode until the End code.
  OpCode Code;
  do {
    uint32_t Offset = Mgr.getOffset();

    /// Read the opcode and check if error.
    if (auto Res = loadOpCode(Mgr)) {
      Code = *Res;
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }

    /// Process the Else OpCode.
    if (Code == OpCode::Else) {
      if (IsElseOpOccurred || MeasureElseOp == nullptr) {
        LOG(ERROR) << ErrCode::InvalidGrammar;
        LOG(ERROR) << ErrInfo::InfoLoading(Offset);
        LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
        return Unexpect(ErrCode::InvalidGrammar);
      }
      IsElseOpOccurred = true;
      *MeasureElseOp = Instrs.size();
      continue;
    }

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if (auto Res = makeInstructionNode(Code, Offset)) {
      NewInst = std::move(*Res);
    } else {
      LOG(ERROR) << ErrInfo::InfoLoading(Offset);
      LOG(ERROR) << ErrInfo::InfoInstruction(Code, Offset);
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }
    if (auto Res = NewInst->loadBinary(Mgr)) {
      Instrs.push_back(std::move(NewInst));
    } else {
      return Unexpect(Res);
    }

  } while (Code != OpCode::End);
  return Instrs;
}

/// Instruction node maker. See "include/ast/instruction.h".
Expect<std::unique_ptr<Instruction>> makeInstructionNode(OpCode Code,
                                                         uint32_t Offset) {
  return dispatchInstruction(
      Code,
      [&Code, &Offset](auto &&Arg) -> Expect<std::unique_ptr<Instruction>> {
        using InstrT = typename std::decay_t<decltype(Arg)>::type;
        if constexpr (std::is_void_v<InstrT>) {
          /// If the Code not matched, return error.
          LOG(ERROR) << ErrCode::InvalidGrammar;
          return Unexpect(ErrCode::InvalidOpCode);
        } else {
          /// Make the instruction node according to Code.
          return std::make_unique<InstrT>(Code, Offset);
        }
      });
}

/// Instruction node duplicater. See "include/ast/instruction.h".
Expect<std::unique_ptr<Instruction>>
makeInstructionNode(const Instruction &Instr) {
  return dispatchInstruction(
      Instr.getOpCode(),
      [&Instr](auto &&Arg) -> Expect<std::unique_ptr<Instruction>> {
        if constexpr (std::is_void_v<
                          typename std::decay_t<decltype(Arg)>::type>) {
          /// If the Code not matched, return null pointer.
          LOG(ERROR) << ErrCode::InvalidGrammar;
          return Unexpect(ErrCode::InvalidGrammar);
        } else {
          /// Make the instruction node according to Code.
          return std::make_unique<typename std::decay_t<decltype(Arg)>::type>(
              static_cast<const typename std::decay_t<decltype(Arg)>::type &>(
                  Instr));
        }
      });
}

} // namespace AST
} // namespace SSVM
