// SPDX-License-Identifier: Apache-2.0
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

namespace WasmEdge {

static Proposal ProposalList[] = {Proposal::Annotations,
                                  Proposal::BulkMemoryOperations,
                                  Proposal::ExceptionHandling,
                                  Proposal::FunctionReferences,
                                  Proposal::Memory64,
                                  Proposal::ReferenceTypes,
                                  Proposal::SIMD,
                                  Proposal::TailCall,
                                  Proposal::Threads};

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
  return static_cast<ErrCode>(Res.Code);
}

std::vector<ValVariant> convToValVec(const std::vector<WasmEdge_Value> &CVals) {
  std::vector<ValVariant> Vals(CVals.size());
  std::transform(
      CVals.cbegin(), CVals.cend(), Vals.begin(),
      [](const WasmEdge_Value &Val) { return ValVariant(Val.Value); });
  return Vals;
}
std::vector<WasmEdge_Value> convFromValVec(const std::vector<ValVariant> &Vals,
                                           const std::vector<ValType> &Types) {
  std::vector<WasmEdge_Value> CVals(Vals.size());
  for (uint32_t I = 0; I < Vals.size(); I++) {
    CVals[I] = WasmEdge_Value{.Value = Vals[I].get<unsigned __int128>(),
                              .Type = static_cast<WasmEdge_ValType>(Types[I])};
  }
  return CVals;
}

} // namespace WasmEdge
