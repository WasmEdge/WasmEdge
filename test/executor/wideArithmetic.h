// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/module.h"

#include <cstdint>
#include <vector>

namespace WasmEdge::Test {

inline AST::Module
makeWideArithmeticModule(OpCode Opcode, const std::vector<ValType> &Params) {
  AST::Module Mod;
  Mod.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Mod.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};
  AST::FunctionType Type;
  Type.getParamTypes() = Params;
  Type.getReturnTypes() = {TypeCode::I64, TypeCode::I64};
  Mod.getTypeSection().getContent().emplace_back(Type);
  Mod.getFunctionSection().getContent().push_back(0U);
  AST::ExportDesc Export;
  Export.setExternalName("wide");
  Export.setExternalType(ExternalType::Function);
  Export.setExternalIndex(0U);
  Mod.getExportSection().getContent().push_back(Export);
  AST::CodeSegment Code;
  for (uint32_t I = 0; I < Params.size(); ++I) {
    AST::Instruction Get(OpCode::Local__get);
    Get.getTargetIndex() = I;
    Code.getExpr().getInstrs().push_back(Get);
  }
  Code.getExpr().getInstrs().emplace_back(Opcode);
  Code.getExpr().getInstrs().emplace_back(OpCode::End);
  Mod.getCodeSection().getContent().push_back(Code);
  return Mod;
}

} // namespace WasmEdge::Test
