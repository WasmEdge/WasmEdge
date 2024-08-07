// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/api/helper.cpp - Spec test helpers for C API --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//

#include "helper.h"

#include <algorithm>
#include <cstdint>
#include <memory>

namespace WasmEdge {

static Proposal ProposalList[] = {
    Proposal::TailCall, Proposal::MultiMemories,      Proposal::Annotations,
    Proposal::Memory64, Proposal::ExceptionHandling,  Proposal::ExtendedConst,
    Proposal::Threads,  Proposal::FunctionReferences, Proposal::GC,
    Proposal::RelaxSIMD};

WasmEdge_ConfigureContext *createConf(const Configure &Conf) {
  auto *Cxt = WasmEdge_ConfigureCreate();
  for (auto &I : ProposalList) {
    if (Conf.hasProposal(I)) {
      WasmEdge_ConfigureAddProposal(Cxt, static_cast<WasmEdge_Proposal>(I));
    }
  }
  return Cxt;
}

ErrCode convResult(WasmEdge_Result Res) {
  return static_cast<ErrCode::Value>(WasmEdge_ResultGetCode(Res));
}

std::pair<ValVariant, ValType> convToVal(const WasmEdge_Value &CVal) {
  std::array<uint8_t, 8> R;
  std::copy_n(CVal.Type.Data, 8, R.begin());
#if defined(__x86_64__) || defined(__aarch64__)
  return std::make_pair(ValVariant(CVal.Value), ValType(R));
#else
  return std::make_pair(
      ValVariant(WasmEdge::uint128_t(CVal.Value.High, CVal.Value.Low)),
      ValType(R));
#endif
}

WasmEdge_Value convFromVal(const ValVariant &Val, const ValType &Type) {
  WasmEdge_Value CVal;
  std::copy_n(Type.getRawData().cbegin(), 8, CVal.Type.Data);
#if defined(__x86_64__) || defined(__aarch64__)
  CVal.Value = Val.get<WasmEdge::uint128_t>();
#else
  WasmEdge::uint128_t U128 = Val.get<WasmEdge::uint128_t>();
  CVal.Value.Low = U128.low();
  CVal.Value.High = static_cast<uint64_t>(U128.high());
#endif
  return CVal;
}

std::vector<std::pair<ValVariant, ValType>>
convToValVec(const std::vector<WasmEdge_Value> &CVals) {
  std::vector<std::pair<ValVariant, ValType>> Vals(CVals.size());
  std::transform(CVals.cbegin(), CVals.cend(), Vals.begin(),
                 [](const WasmEdge_Value &Val) { return convToVal(Val); });
  return Vals;
}

std::vector<WasmEdge_Value> convFromValVec(const std::vector<ValVariant> &Vals,
                                           const std::vector<ValType> &Types) {
  std::vector<WasmEdge_Value> CVals(Vals.size());
  for (uint32_t I = 0; I < Vals.size(); I++) {
    CVals[I] = convFromVal(Vals[I], Types[I]);
  }
  return CVals;
}

} // namespace WasmEdge
