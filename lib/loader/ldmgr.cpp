// SPDX-License-Identifier: Apache-2.0
#include "loader/ldmgr.h"
#include "common/log.h"

namespace WasmEdge {

/// Set path to loadable manager. See "include/loader/ldmgr.h".
Expect<void> LDMgr::setPath(const std::filesystem::path &FilePath) {
  Library = std::make_shared<Loader::SharedLibrary>();
  return Library->load(FilePath);
}

Expect<std::vector<Byte>> LDMgr::getWasm() {
  const auto Size = getSymbol<uint32_t>("wasm.size");
  if (!Size) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    return Unexpect(ErrCode::InvalidGrammar);
  }
  const auto Code = getSymbol<uint8_t>("wasm.code");
  if (!Code) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    return Unexpect(ErrCode::InvalidGrammar);
  }

  return std::vector<Byte>(Code.get(), Code.get() + *Size);
}

Expect<uint32_t> LDMgr::getVersion() {
  const auto Version = getSymbol<uint32_t>("version");
  if (!Version) {
    LOG(ERROR) << ErrCode::InvalidGrammar;
    return Unexpect(ErrCode::InvalidGrammar);
  }
  return *Version;
}

} // namespace WasmEdge
