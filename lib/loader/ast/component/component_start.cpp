// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadStart(AST::Component::Start &S) {
  auto ReportError = [this](auto E) {
    return logLoadError(E, FMgr.getLastOffset(), ASTNodeAttr::Comp_Start);
  };
  // start ::= f:<funcidx> arg*:vec(<valueidx>) r:<u32>
  //         => (start f (value arg)* (result (value))ʳ)

  EXPECTED_TRY(S.getFunctionIndex(), FMgr.readU32().map_error(ReportError));
  auto F = [this, ReportError](uint32_t &V) -> Expect<void> {
    EXPECTED_TRY(V, FMgr.readU32().map_error(ReportError));
    return {};
  };
  EXPECTED_TRY(loadVec<AST::Component::Start>(S.getArguments(), F));
  EXPECTED_TRY(S.getResult(), FMgr.readU32().map_error(ReportError));
  return {};
}

} // namespace Loader
} // namespace WasmEdge
