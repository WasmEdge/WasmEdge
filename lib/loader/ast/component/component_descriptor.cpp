// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadDesc(AST::Component::CoreImportDesc &Desc) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(),
                        ASTNodeAttr::Comp_Desc_CoreImport);
  };
  // core:importdesc ::= 0x00 x:typeidx => (func i)
  //                   | 0x01 tt:tabletype => (table tt)
  //                   | 0x02 mt:memtype => (mem mt)
  //                   | 0x03 gt:globaltype => (global gt)
  //                   | 0x04 tt:tagtype => (tag tt)

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (static_cast<ExternalType>(Flag)) {
  case ExternalType::Function: {
    // Read the function type index.
    EXPECTED_TRY(FMgr.readU32().map_error(ReportError).map([&](uint32_t Idx) {
      Desc.setTypeIndex(Idx);
    }));
    return {};
  }
  case ExternalType::Table: {
    AST::TableType TT;
    EXPECTED_TRY(loadType(TT).map_error(ReportError));
    Desc.setTableType(std::move(TT));
    return {};
  }
  case ExternalType::Memory: {
    AST::MemoryType MT;
    EXPECTED_TRY(loadType(MT).map_error(ReportError));
    Desc.setMemoryType(std::move(MT));
    return {};
  }
  case ExternalType::Global: {
    AST::GlobalType GT;
    EXPECTED_TRY(loadType(GT).map_error(ReportError));
    Desc.setGlobalType(std::move(GT));
    return {};
  }
  case ExternalType::Tag: {
    AST::TagType TT;
    EXPECTED_TRY(loadType(TT).map_error(ReportError));
    Desc.setTagType(std::move(TT));
    return {};
  }
  default:
    return ReportError(ErrCode::Value::IllegalGrammar);
  }
}

Expect<void> Loader::loadDesc(AST::Component::ExternDesc &Desc) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Desc_Extern);
  };
  // externdesc ::= 0x00 0x11 i:<core:typeidx> => (core module (type i))
  //              | 0x01 i:<typeidx>           => (func (type i))
  //              | 0x02 b:<valuebound>        => (value b) 🪙
  //              | 0x03 b:<typebound>         => (type b)
  //              | 0x04 i:<typeidx>           => (component (type i))
  //              | 0x05 i:<typeidx>           => (instance (type i))
  // valuebound ::= 0x00 i:<valueidx>          => (eq i) 🪙
  //              | 0x01 t:<valtype>           => t 🪙
  // typebound  ::= 0x00 i:<typeidx>           => (eq i)
  //              | 0x01                       => (sub resource)

  EXPECTED_TRY(uint8_t Flag, FMgr.readByte().map_error(ReportError));
  switch (Flag) {
  case 0x00: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B != 0x11U) {
      return ReportError(ErrCode::Value::IntegerTooLong);
    }
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    Desc.setCoreTypeIdx(Idx);
    return {};
  }
  case 0x01: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    Desc.setFuncTypeIdx(Idx);
    return {};
  }
  case 0x02: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B == 0x00) {
      EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
      Desc.setValueBound(Idx);
    } else if (B == 0x01) {
      AST::Component::ValueType VT;
      EXPECTED_TRY(loadType(VT).map_error(ReportError));
      Desc.setValueBound(VT);
    } else {
      return ReportError(ErrCode::Value::IllegalGrammar);
    }
    return {};
  }
  case 0x03: {
    EXPECTED_TRY(uint8_t B, FMgr.readByte().map_error(ReportError));
    if (B == 0x00) {
      EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
      Desc.setTypeBound(Idx);
    } else if (B == 0x01) {
      Desc.setTypeBound();
    } else {
      return ReportError(ErrCode::Value::IllegalGrammar);
    }
    return {};
  }
  case 0x04: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    Desc.setComponentTypeIdx(Idx);
    return {};
  }
  case 0x05: {
    EXPECTED_TRY(uint32_t Idx, FMgr.readU32().map_error(ReportError));
    Desc.setInstanceTypeIdx(Idx);
    return {};
  }
  default:
    return ReportError(ErrCode::Value::IllegalGrammar);
  }
}

} // namespace Loader
} // namespace WasmEdge
