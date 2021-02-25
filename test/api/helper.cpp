// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/api/helper.cpp - Spec test helpers for C API ------------===//
//
// Part of the SSVM Project.
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

namespace SSVM {

static Proposal ProposalList[] = {Proposal::Annotations,
                                  Proposal::BulkMemoryOperations,
                                  Proposal::ExceptionHandling,
                                  Proposal::FunctionReferences,
                                  Proposal::Memory64,
                                  Proposal::ReferenceTypes,
                                  Proposal::SIMD,
                                  Proposal::TailCall,
                                  Proposal::Threads};

SSVM_ConfigureContext *createConf(const Configure &Conf) {
  auto *Cxt = SSVM_ConfigureCreate();
  for (auto &I : ProposalList) {
    if (Conf.hasProposal(I)) {
      SSVM_ConfigureAddProposal(Cxt, static_cast<SSVM_Proposal>(I));
    }
  }
  return Cxt;
}

ErrCode convResult(SSVM_Result Res) { return static_cast<ErrCode>(Res.Code); }

std::vector<ValVariant> convToValVec(const std::vector<SSVM_Value> &CVals) {
  std::vector<ValVariant> Vals(CVals.size());
  std::transform(CVals.cbegin(), CVals.cend(), Vals.begin(),
                 [](const SSVM_Value &Val) { return ValVariant(Val.Value); });
  return Vals;
}
std::vector<SSVM_Value> convFromValVec(const std::vector<ValVariant> &Vals,
                                       const std::vector<ValType> &Types) {
  std::vector<SSVM_Value> CVals(Vals.size());
  for (uint32_t I = 0; I < Vals.size(); I++) {
    CVals[I] = SSVM_Value{.Value = retrieveValue<unsigned __int128>(Vals[I]),
                          .Type = static_cast<SSVM_ValType>(Types[I])};
  }
  return CVals;
}

} // namespace SSVM
