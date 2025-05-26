// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

using namespace AST::Component;

Expect<void> Loader::loadStart(Start &S) {
  EXPECTED_TRY(S.getFunctionIndex(), FMgr.readU32());

  auto F = [this](uint32_t &V) -> Expect<void> {
    EXPECTED_TRY(V, FMgr.readU32());
    return {};
  };
  EXPECTED_TRY(loadVec<StartSection>(S.getArguments(), F));
  EXPECTED_TRY(S.getResult(), FMgr.readU32());
  return {};
}

} // namespace Loader
} // namespace WasmEdge
