// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "ast/module.h"

#include <array>
#include <cstdint>
#include <vector>

namespace WasmEdge::Test {

inline AST::Module makeWideArithmeticModule(
    OpCode Opcode, const std::vector<ValType> &Params,
    const std::vector<ValType> &Returns = {TypeCode::I64, TypeCode::I64}) {
  AST::Module Mod;
  Mod.getMagic() = {0x00U, 0x61U, 0x73U, 0x6DU};
  Mod.getVersion() = {0x01U, 0x00U, 0x00U, 0x00U};
  AST::FunctionType Type;
  Type.getParamTypes() = Params;
  Type.getReturnTypes() = Returns;
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

struct WideArithmeticCase {
  OpCode Opcode;
  // 128-bit operands and results store the low 64 bits before the high 64 bits.
  std::vector<uint64_t> Params;
  std::array<uint64_t, 2> Results;
};

inline const std::vector<WideArithmeticCase> WideArithmeticCases = {
    {OpCode::I64__add128, {UINT64_MAX, 0U, 1U, 0U}, {0U, 1U}},
    {OpCode::I64__add128, {UINT64_MAX, UINT64_MAX, 1U, 0U}, {0U, 0U}},
    {OpCode::I64__add128, {2U, 3U, 5U, 7U}, {7U, 10U}},
    {OpCode::I64__add128,
     {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX},
     {UINT64_MAX - 1U, UINT64_MAX}},
    {OpCode::I64__add128, {0U, UINT64_MAX, 0U, 1U}, {0U, 0U}},
    {OpCode::I64__add128,
     {UINT64_C(0x7fffffffffffffff), 3U, 1U, 7U},
     {UINT64_C(0x8000000000000000), 10U}},
    {OpCode::I64__sub128, {0U, 1U, 1U, 0U}, {UINT64_MAX, 0U}},
    {OpCode::I64__sub128, {0U, 0U, 1U, 0U}, {UINT64_MAX, UINT64_MAX}},
    {OpCode::I64__sub128, {9U, 7U, 2U, 3U}, {7U, 4U}},
    {OpCode::I64__sub128, {0U, 0U, UINT64_MAX, UINT64_MAX}, {1U, 0U}},
    {OpCode::I64__sub128, {UINT64_MAX, 0U, UINT64_MAX, 1U}, {0U, UINT64_MAX}},
    {OpCode::I64__sub128,
     {UINT64_C(0x8000000000000000), 7U, 1U, 3U},
     {UINT64_C(0x7fffffffffffffff), 4U}},
};

} // namespace WasmEdge::Test
