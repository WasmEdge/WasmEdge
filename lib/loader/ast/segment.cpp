// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Loader {

// Load binary of TableSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::TableSegment &TabSeg) {
  // Check the first byte is the reftype in table type or not.
  if (auto CheckByte = FMgr.peekByte()) {
    if (*CheckByte == 0x40U) {
      // Table segment case is for FunctionReferences proposal.
      if (!Conf.hasProposal(Proposal::FunctionReferences)) {
        return logNeedProposal(ErrCode::Value::MalformedTable,
                               Proposal::FunctionReferences,
                               FMgr.getLastOffset(), ASTNodeAttr::Seg_Table);
      }
      FMgr.readByte();

      // Check the second byte.
      if (auto Res = FMgr.readByte()) {
        if (*Res != 0x00U) {
          return logLoadError(ErrCode::Value::MalformedTable,
                              FMgr.getLastOffset(), ASTNodeAttr::Seg_Table);
        }
      } else {
        return logLoadError(Res.error(), FMgr.getLastOffset(),
                            ASTNodeAttr::Seg_Table);
      }

      // Read the table type.
      if (auto Res = loadType(TabSeg.getTableType()); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
        return Unexpect(Res);
      }

      // Read the expression.
      if (auto Res = loadExpression(TabSeg.getExpr()); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
        return Unexpect(Res);
      }
    } else {
      // The table type case.
      if (auto Res = loadType(TabSeg.getTableType()); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
        return Unexpect(Res);
      }
    }
  } else {
    return logLoadError(CheckByte.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Table);
  }

  return {};
}

// Load binary of GlobalSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::GlobalSegment &GlobSeg) {
  // Read global type node.
  if (auto Res = loadType(GlobSeg.getGlobalType()); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
    return Unexpect(Res);
  }

  // Read the expression.
  if (auto Res = loadExpression(GlobSeg.getExpr()); unlikely(!Res)) {
    spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
    return Unexpect(Res);
  }

  return {};
}

// Load binary of ElementSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::ElementSegment &ElemSeg) {
  // Element segment binary format:
  // ---------------------------------------------------------------------------
  //  Mode | TableIdx | OffExpr | ElemKind | RefType | vec(FuncIdx) | vec(expr)
  // ------|----------|---------|----------|---------|--------------|-----------
  //    0  |          |    v    |          |         |       v      |
  //    1  |          |         |    v     |         |       v      |
  //    2  |    v     |    v    |    v     |         |       v      |
  //    3  |          |         |    v     |         |       v      |
  //    4  |          |    v    |          |         |              |     v
  //    5  |          |         |          |    v    |              |     v
  //    6  |    v     |    v    |          |    v    |              |     v
  //    7  |          |         |          |    v    |              |     v
  // ---------------------------------------------------------------------------
  // Mode: element initial integer, u32
  // TableIdx: target table index, u32
  // OffExpr: init offset expression, expr
  // ElemKind: byte 0x00, RefType::FuncRef
  // RefType: reference type, RefType
  // vec(FuncIdx): function index vector, vec(u32)
  // vec(expr): reference init list, vec(expr)

  // Read the checking byte.
  uint32_t Check;
  if (auto Res = FMgr.readU32()) {
    Check = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Element);
  }
  // Check > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Check > 0 && !Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    return logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                           Proposal::BulkMemoryOperations, FMgr.getLastOffset(),
                           ASTNodeAttr::Seg_Element);
  }

  // Check the prefix byte.
  switch (Check) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
    ElemSeg.setMode(AST::ElementSegment::ElemMode::Active);
    break;

  case 0x01:
  case 0x05:
    ElemSeg.setMode(AST::ElementSegment::ElemMode::Passive);
    break;

  case 0x03:
  case 0x07:
    ElemSeg.setMode(AST::ElementSegment::ElemMode::Declarative);
    break;

  default:
    // TODO: Correctness the error code once there's spec test.
    return logLoadError(ErrCode::Value::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Element);
  }

  // Read the table index.
  ElemSeg.setIdx(0);
  switch (Check) {
  case 0x02:
  case 0x06:
    if (auto Res = FMgr.readU32()) {
      ElemSeg.setIdx(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    }
    break;

  default:
    break;
  }

  // Read the expression.
  switch (Check) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
    if (auto Res = loadExpression(ElemSeg.getExpr()); unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
      return Unexpect(Res);
    }
    break;

  default:
    break;
  }

  // Read element kind and init function indices.
  ElemSeg.setRefType(TypeCode::FuncRef);
  switch (Check) {
  case 0x01:
  case 0x02:
  case 0x03:
    if (auto Res = FMgr.readByte()) {
      if (*Res != 0x00U) {
        return logLoadError(ErrCode::Value::ExpectedZeroByte,
                            FMgr.getLastOffset(), ASTNodeAttr::Seg_Element);
      }
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    }
    [[fallthrough]];

  case 0x00: {
    uint32_t VecCnt = 0;
    if (auto Res = FMgr.readU32()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Element);
    }
    for (uint32_t I = 0; I < VecCnt; ++I) {
      // For each element in vec(funcidx), make expr(ref.func idx end).
      ElemSeg.getInitExprs().emplace_back();
      AST::Instruction RefFunc(OpCode::Ref__func);
      AST::Instruction End(OpCode::End);
      if (auto Res = loadInstruction(RefFunc); unlikely(!Res)) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
        return Unexpect(Res);
      }
      ElemSeg.getInitExprs().back().getInstrs().emplace_back(
          std::move(RefFunc));
      ElemSeg.getInitExprs().back().getInstrs().emplace_back(std::move(End));
    }
    break;
  }
  default:
    break;
  }

  // Read the reference type and init expressions.
  switch (Check) {
  case 0x05:
  case 0x06:
  case 0x07:
    if (auto Res = loadRefType(ASTNodeAttr::Seg_Element)) {
      ElemSeg.setRefType(*Res);
    } else {
      // The AST node information is handled.
      return Unexpect(Res);
    }
    [[fallthrough]];
  case 0x04: {
    return loadVec<AST::ElementSegment>(
        ElemSeg.getInitExprs(), [this](AST::Expression &Expr) -> Expect<void> {
          return loadExpression(Expr);
        });
  }

  default:
    break;
  }

  return {};
}

// Load binary of CodeSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::CodeSegment &CodeSeg) {
  // Read the code segment size.
  if (auto Res = FMgr.readU32()) {
    CodeSeg.setSegSize(*Res);
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Code);
  }
  auto ExprSizeBound = FMgr.getOffset() + CodeSeg.getSegSize();

  // Read the vector of local variable counts and types.
  uint32_t VecCnt = 0;
  if (auto Res = loadVecCnt()) {
    VecCnt = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Element);
  }
  CodeSeg.getLocals().clear();
  CodeSeg.getLocals().reserve(VecCnt);
  uint32_t TotalLocalCnt = 0;
  for (uint32_t I = 0; I < VecCnt; ++I) {
    uint32_t LocalCnt = 0;
    ValType LocalType;
    if (auto Res = FMgr.readU32(); unlikely(!Res)) {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Code);
    } else {
      LocalCnt = *Res;
    }
    // Total local variables should not more than 2^32. Capped at 2^26.
    if (UINT32_C(67108864) - TotalLocalCnt < LocalCnt) {
      return logLoadError(ErrCode::Value::TooManyLocals, FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Code);
    }
    TotalLocalCnt += LocalCnt;
    // Read the value type.
    if (auto Res = loadValType(ASTNodeAttr::Seg_Code)) {
      LocalType = *Res;
    } else {
      // The AST node information is handled.
      return Unexpect(Res);
    }
    CodeSeg.getLocals().push_back(std::make_pair(LocalCnt, LocalType));
  }

  if (!Conf.getRuntimeConfigure().isForceInterpreter() &&
      WASMType != InputType::WASM) {
    // For the AOT mode and not force interpreter in configure, skip the
    // function body.
    FMgr.seek(ExprSizeBound);
  } else {
    // Read function body with expected expression size.
    if (auto Res = loadExpression(CodeSeg.getExpr(), ExprSizeBound);
        unlikely(!Res)) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Code));
      return Unexpect(Res);
    }
  }

  return {};
}

// Load binary of DataSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::DataSegment &DataSeg) {
  DataSeg.setMode(AST::DataSegment::DataMode::Passive);
  DataSeg.setIdx(0);

  // Data segment binary format:
  // ----------------------------------------
  //  Mode | MemoryIdx | OffExpr | vec(byte)
  // ------|-----------|---------|-----------
  //    0  |           |    v    |     v
  //    1  |           |         |     v
  //    2  |     v     |    v    |     v
  // ----------------------------------------
  // Mode: data initial integer, u32
  // MemoryIdx: target memory index, u32
  // OffExpr: init offset expression, expr
  // vec(byte): init data, vec(u8)

  // Read the checking byte.
  uint32_t Check;
  if (auto Res = FMgr.readU32()) {
    Check = *Res;
  } else {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Data);
  }
  // Check > 0 cases are for BulkMemoryOperations or ReferenceTypes proposal.
  if (Check > 0 && !Conf.hasProposal(Proposal::BulkMemoryOperations) &&
      !Conf.hasProposal(Proposal::ReferenceTypes)) {
    return logNeedProposal(ErrCode::Value::ExpectedZeroByte,
                           Proposal::BulkMemoryOperations, FMgr.getLastOffset(),
                           ASTNodeAttr::Seg_Data);
  }

  switch (Check) {
  case 0x02: // 0x02 memidx expr vec(byte) , Active
    // Read target memory index.
    if (auto Res = FMgr.readU32()) {
      DataSeg.setIdx(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Data);
    }
    [[fallthrough]];

  case 0x00: // 0x00 expr vec(byte) , Active
    // Read the offset expression.
    if (auto Res = loadExpression(DataSeg.getExpr()); !Res) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
      return Unexpect(Res);
    }
    DataSeg.setMode(AST::DataSegment::DataMode::Active);
    [[fallthrough]];

  case 0x01: // 0x01 vec(byte) , Passive
  {
    // Read initialization data.
    uint32_t VecCnt = 0;
    if (auto Res = loadVecCnt()) {
      VecCnt = *Res;
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Data);
    }
    if (auto Res = FMgr.readBytes(VecCnt)) {
      DataSeg.getData() = std::move(*Res);
    } else {
      return logLoadError(Res.error(), FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Data);
    }
    break;
  }
  default:
    // TODO: Correctness the error code once there's spec test.
    return logLoadError(ErrCode::Value::IllegalGrammar, FMgr.getLastOffset(),
                        ASTNodeAttr::Seg_Data);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
