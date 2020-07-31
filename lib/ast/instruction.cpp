// SPDX-License-Identifier: Apache-2.0
#include "common/ast/instruction.h"
#include "support/log.h"

namespace SSVM {
namespace AST {

/// Copy construtor. See "include/common/ast/instruction.h".
BlockControlInstruction::BlockControlInstruction(
    const BlockControlInstruction &Instr)
    : Instruction(Instr.Code, Instr.Offset), ResType(Instr.ResType) {
  for (auto &It : Instr.Body) {
    if (auto Res = makeInstructionNode(*It.get())) {
      Body.push_back(std::move(*Res));
    }
  }
}

/// Load binary of block instructions. See "include/common/ast/instruction.h".
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

  /// Read instructions and make nodes until Opcode::End.
  while (true) {
    OpCode Code;
    uint32_t Offset = Mgr.getOffset();

    /// Read the opcode and check if error.
    if (auto Res = loadOpCode(Mgr)) {
      Code = *Res;
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }

    /// When reach end, this block is ended.
    if (Code == OpCode::End) {
      break;
    }

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if (auto Res = makeInstructionNode(Code, Offset)) {
      NewInst = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
    if (auto Res = NewInst->loadBinary(Mgr)) {
      Body.push_back(std::move(NewInst));
    } else {
      return Unexpect(Res);
    }
  }

  return {};
}

/// Copy construtor. See "include/common/ast/instruction.h".
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

/// Load binary of if-else instructions. See "include/common/ast/instruction.h".
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
  bool IsElseStatement = false;
  while (true) {
    OpCode Code;
    uint32_t Offset = Mgr.getOffset();

    /// Read the opcode and check if error.
    if (auto Res = loadOpCode(Mgr)) {
      Code = *Res;
    } else {
      LOG(ERROR) << ErrInfo::InfoAST(ASTNodeAttr::Instruction);
      return Unexpect(Res);
    }

    /// When reach end, this if-else block is ended.
    if (Code == OpCode::End) {
      break;
    }

    /// If an OpCode::Else read, switch to Else statement.
    if (Code == OpCode::Else) {
      IsElseStatement = true;
      continue;
    }

    /// Create the instruction node and load contents.
    std::unique_ptr<Instruction> NewInst;
    if (auto Res = makeInstructionNode(Code, Offset)) {
      NewInst = std::move(*Res);
    } else {
      return Unexpect(Res);
    }
    if (auto Res = NewInst->loadBinary(Mgr)) {
      if (IsElseStatement) {
        ElseStatement.push_back(std::move(NewInst));
      } else {
        IfStatement.push_back(std::move(NewInst));
      }
    } else {
      return Unexpect(Res);
    }
  }

  return {};
}

/// Load binary of branch instructions. See "include/common/ast/instruction.h".
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

/// Load branch table instructions. See "include/common/ast/instruction.h".
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

/// Load binary of call instructions. See "include/common/ast/instruction.h".
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

/// Load variable instructions. See "include/common/ast/instruction.h".
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

/// Load binary of memory instructions. See "include/common/ast/instruction.h".
Expect<void> MemoryInstruction::loadBinary(FileMgr &Mgr) {
  /// Read the 0x00 checking code in memory.grow and memory.size cases.
  if (Code == OpCode::Memory__grow || Code == OpCode::Memory__size) {
    if (auto Res = Mgr.readByte()) {
      if (*Res == 0x00) {
        return {};
      } else {
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

/// Load const numeric instructions. See "include/common/ast/instruction.h".
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

/// OpCode loader. See "include/common/ast/instruction.h".
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
  }
  return static_cast<OpCode>(Payload);
}

/// Instruction node maker. See "include/common/ast/instruction.h".
Expect<std::unique_ptr<Instruction>> makeInstructionNode(OpCode Code,
                                                         uint32_t Offset) {
  return dispatchInstruction(
      Code,
      [&Code, &Offset](auto &&Arg) -> Expect<std::unique_ptr<Instruction>> {
        if constexpr (std::is_void_v<
                          typename std::decay_t<decltype(Arg)>::type>) {
          /// If the Code not matched, return null pointer.
          LOG(ERROR) << ErrCode::InvalidGrammar;
          return Unexpect(ErrCode::InvalidGrammar);
        } else {
          /// Make the instruction node according to Code.
          return std::make_unique<typename std::decay_t<decltype(Arg)>::type>(
              Code, Offset);
        }
      });
}

/// Instruction node duplicater. See "include/common/ast/instruction.h".
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
