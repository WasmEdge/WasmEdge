// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

// Load binary of TableSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::TableSegment &TabSeg) {
  // Check the first byte is the reftype in table type or not.
  EXPECTED_TRY(auto CheckByte, FMgr.peekByte().map_error([this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Seg_Table)
        .value();
  }));

  if (CheckByte == 0x40U) {
    // Table segment case is for FunctionReferences proposal.
    if (!Conf.hasProposal(Proposal::FunctionReferences)) {
      return logNeedProposal(ErrCode::Value::MalformedTable,
                             Proposal::FunctionReferences, FMgr.getLastOffset(),
                             ASTNodeAttr::Seg_Table);
    }
    FMgr.readByte();

    // Check the second byte.
    EXPECTED_TRY(auto B, FMgr.readByte().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Seg_Table)
          .value();
    }));
    if (B != 0x00U) {
      return logLoadError(ErrCode::Value::MalformedTable, FMgr.getLastOffset(),
                          ASTNodeAttr::Seg_Table);
    }

    // Read the table type.
    EXPECTED_TRY(loadType(TabSeg.getTableType()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
      return E;
    }));

    // Read the expression.
    EXPECTED_TRY(loadExpression(TabSeg.getExpr()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
      return E;
    }));
  } else {
    // The table type case.
    EXPECTED_TRY(loadType(TabSeg.getTableType()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Table));
      return E;
    }));
  }

  return {};
}

// Load binary of GlobalSegment node. See "include/loader/loader.h".
Expect<void> Loader::loadSegment(AST::GlobalSegment &GlobSeg) {
  return Expect<void>{}
      .and_then([this, &GlobSeg]() {
        // Read global type node.
        return loadType(GlobSeg.getGlobalType());
      })
      .and_then([this, &GlobSeg]() {
        // Read the expression.
        return loadExpression(GlobSeg.getExpr());
      })
      .map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Global));
        return E;
      });
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
  // ElemKind: byte 0x00, ref.func
  // RefType: reference type, RefType
  // vec(FuncIdx): function index vector, vec(u32)
  // vec(expr): reference init list, vec(expr)

  // Read the checking byte.
  uint32_t Check = 0;
  if (unlikely(!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
               !Conf.hasProposal(Proposal::ReferenceTypes))) {
    // Legacy for BulkMemoryOperations and ReferenceTypes proposals turned off.
    // Element segment binary format: TableIdx + OffExpr + vec(FuncIdx)
    EXPECTED_TRY(FMgr.readU32()
                     .map_error([this](auto E) {
                       return logLoadError(E, FMgr.getLastOffset(),
                                           ASTNodeAttr::Seg_Element)
                           .value();
                     })
                     .map([&](auto Idx) { ElemSeg.setIdx(Idx); }));
  } else {
    EXPECTED_TRY(Check, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Seg_Element)
          .value();
    }));
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
    EXPECTED_TRY(FMgr.readU32()
                     .map_error([this](auto E) {
                       return logLoadError(E, FMgr.getLastOffset(),
                                           ASTNodeAttr::Seg_Element)
                           .value();
                     })
                     .map([&](auto Idx) { ElemSeg.setIdx(Idx); }));
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
    EXPECTED_TRY(loadExpression(ElemSeg.getExpr()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
      return E;
    }));
    break;

  default:
    break;
  }

  // Read element kind and init function indices.
  switch (Check) {
  case 0x01:
  case 0x02:
  case 0x03:
    EXPECTED_TRY(FMgr.readByte()
                     .and_then([&](auto B) -> Expect<void> {
                       if (B != 0x00U) {
                         return Unexpect(ErrCode::Value::ExpectedZeroByte);
                       };
                       return {};
                     })
                     .map_error([this](auto E) {
                       return logLoadError(E, FMgr.getLastOffset(),
                                           ASTNodeAttr::Seg_Element)
                           .value();
                     }));
    [[fallthrough]];

  case 0x00: {
    EXPECTED_TRY(uint32_t VecCnt, FMgr.readU32().map_error([this](auto E) {
      return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Seg_Element)
          .value();
    }));
    for (uint32_t I = 0; I < VecCnt; ++I) {
      // For each element in vec(funcidx), make expr(ref.func idx end).
      ElemSeg.getInitExprs().emplace_back();
      AST::Instruction RefFunc(OpCode::Ref__func);
      AST::Instruction End(OpCode::End);
      EXPECTED_TRY(loadInstruction(RefFunc).map_error([](auto E) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Element));
        return E;
      }));
      ElemSeg.getInitExprs().back().getInstrs().emplace_back(
          std::move(RefFunc));
      ElemSeg.getInitExprs().back().getInstrs().emplace_back(std::move(End));
    }
    break;
  }
  default:
    break;
  }

  // Set the default reference type.
  if (Check == 0x04) {
    ElemSeg.setRefType(TypeCode::FuncRef);
  } else {
    ElemSeg.setRefType(ValType(TypeCode::Ref, TypeCode::FuncRef));
  }

  // Read the reference type and init expressions.
  switch (Check) {
  case 0x05:
  case 0x06:
  case 0x07: {
    // The AST node information is handled.
    EXPECTED_TRY(auto Type, loadRefType(ASTNodeAttr::Seg_Element));
    ElemSeg.setRefType(Type);
    [[fallthrough]];
  }
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
    // The AST node information is handled.
    EXPECTED_TRY(ValType LocalType, loadValType(ASTNodeAttr::Seg_Code));
    CodeSeg.getLocals().push_back(std::make_pair(LocalCnt, LocalType));
  }

  if (!Conf.getRuntimeConfigure().isForceInterpreter() &&
      WASMType != InputType::WASM) {
    // For the AOT mode and not force interpreter in configure, skip the
    // function body.
    FMgr.seek(ExprSizeBound);
  } else {
    // Read function body with expected expression size.
    EXPECTED_TRY(
        loadExpression(CodeSeg.getExpr(), ExprSizeBound).map_error([](auto E) {
          spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Code));
          return E;
        }));
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
    EXPECTED_TRY(loadExpression(DataSeg.getExpr()).map_error([](auto E) {
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Seg_Data));
      return E;
    }));
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
