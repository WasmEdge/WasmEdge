// SPDX-License-Identifier: Apache-2.0
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) ||                \
    defined(__TOS_WIN__) || defined(__WINDOWS__)
#error windows is not supported yet!
#endif

#include "loader/ldmgr.h"
#include "support/log.h"

#include <dlfcn.h>

namespace SSVM {

/// Open so file. See "include/loader/ldmgr.h".
std::shared_ptr<DLHandle> DLHandle::open(const char *Path) {
  auto Result = std::make_shared<DLHandle>();
  Result->Handle = dlopen(Path, RTLD_LAZY | RTLD_LOCAL);
  if (!Result->Handle) {
    Result.reset();
  }
  return Result;
}

/// Get address of a symbol. See "include/loader/ldmgr.h".
void *DLHandle::getRawSymbol(const char *Name) noexcept {
  return dlsym(Handle, Name);
}

/// Close so file. See "include/loader/ldmgr.h".
DLHandle::~DLHandle() noexcept {
  if (Handle) {
    dlclose(Handle);
  }
}

/// Set path to loadable manager. See "include/loader/ldmgr.h".
Expect<void> LDMgr::setPath(std::string_view FilePath) {
  Handle = DLHandle::open(std::string(FilePath).c_str());
  if (!Handle) {
    LOG(ERROR) << ErrCode::InvalidPath;
    return Unexpect(ErrCode::InvalidPath);
  }
  return {};
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

} // namespace SSVM
