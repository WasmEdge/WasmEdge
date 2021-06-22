// SPDX-License-Identifier: Apache-2.0
#include "loader/ldmgr.h"
#include "common/log.h"

namespace WasmEdge {

/// Set path to loadable manager. See "include/loader/ldmgr.h".
Expect<void> LDMgr::setPath(const std::filesystem::path &FilePath) {
  Library = std::make_shared<Loader::SharedLibrary>();
  if (auto Res = Library->load(FilePath); unlikely(!Res)) {
    return Unexpect(Res);
  }

  const auto Table = getSymbol<void(const void *)>("init");
  if (unlikely(!Table)) {
    spdlog::error(ErrCode::IllegalGrammar);
    return Unexpect(ErrCode::IllegalGrammar);
  }

  Table(Intrinsics);
  return {};
}

Expect<std::vector<Byte>> LDMgr::getWasm() {
  const auto Size = getSymbol<uint32_t>("wasm.size");
  if (unlikely(!Size)) {
    spdlog::error(ErrCode::IllegalGrammar);
    return Unexpect(ErrCode::IllegalGrammar);
  }
  const auto Code = getSymbol<uint8_t>("wasm.code");
  if (unlikely(!Code)) {
    spdlog::error(ErrCode::IllegalGrammar);
    return Unexpect(ErrCode::IllegalGrammar);
  }

  return std::vector<Byte>(Code.get(), Code.get() + *Size);
}

Expect<uint32_t> LDMgr::getVersion() {
  const auto Version = getSymbol<uint32_t>("version");
  if (unlikely(!Version)) {
    spdlog::error(ErrCode::IllegalGrammar);
    return Unexpect(ErrCode::IllegalGrammar);
  }
  return *Version;
}

} // namespace WasmEdge
