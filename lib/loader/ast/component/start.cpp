// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC
#include "loader/loader.h"

namespace WasmEdge {
namespace Loader {

Expect<void> Loader::loadStart(AST::Start &S) {
  if (auto Res = FMgr.readU32()) {
    S.getFunctionIndex() = *Res;
  } else {
    return Unexpect(Res);
  }

  auto F = [this](uint32_t &V) -> Expect<void> {
    if (auto Res = FMgr.readU32()) {
      V = *Res;
      return {};
    } else {
      return Unexpect(Res);
    }
  };
  if (auto Res = loadVec<AST::CompStartSection>(S.getArguments(), F); !Res) {
    return Unexpect(Res);
  }

  if (auto Res = FMgr.readU32()) {
    S.getResult() = *Res;
  } else {
    return Unexpect(Res);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
