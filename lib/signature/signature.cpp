// SPDX-License-Identifier: Apache-2.0
#include "signature/signature.h"
#include "common/enum_errcode.h"
#include "common/errcode.h"
#include "spdlog/spdlog.h"

namespace WasmEdge {
namespace Signature {

Expect<void> Signature::signWasmFile(const std::filesystem::path &Path) {
  Span<Byte> Code;
  FileMgr FMgr;
  if (std::ifstream Is{Path, std::ios::binary | std::ios::ate}) {
    auto SizeToRead = Is.tellg();
    FMgr.setPath(Path);
    std::string Str(SizeToRead, '\0');
    if (auto Res = FMgr.readBytes(SizeToRead)) {
      auto *Data = (*Res).data();
      Code = Span<Byte>(reinterpret_cast<Byte *>(*Data), SizeToRead);
    } else
      return Unexpect(ErrCode::IllegalPath);
  }

  if (auto Sig = keygen(Code, Path.parent_path()); !Sig) {
    spdlog::error(ErrInfo::InfoFile(Path));
    return Unexpect(Sig);
  } else
    return sign(Path, *Sig);
}

Expect<bool>
Signature::verifyWasmFile(const std::filesystem::path &Path,
                          const std::filesystem::path &PubKeyPath) {
  LDMgr LMgr;
  Configure Conf;
  Loader::Loader TempLoader(Conf);
  if (auto Res = TempLoader.parseModule(Path)) {
    for (auto CustomSec : (*Res)->getCustomSections()) {
      if (CustomSec.getName() == DEFAULT_CUSOTM_SECTION_NAME) {
        return verify(CustomSec.getContent(), Path, PubKeyPath);
      }
    }
    return Unexpect(Res);
  } else
    return Unexpect(Res);
}

Expect<void> Signature::sign(std::filesystem::path Path,
                             const std::vector<uint8_t> Signature) {
  std::ofstream File(Path.string(), std::ios::binary | std::ios::ate);
  try {
    File.exceptions(File.failbit);
  } catch (const std::ios_base::failure &Error) {
    // Failure handling
  }
  uint8_t SectionId = 0;
  uint8_t SectionLen = Signature.size();
  uint8_t NameLen = sizeof DEFAULT_CUSOTM_SECTION_NAME;
  File.write(reinterpret_cast<const char *>(&SectionId), 1);
  File.write(reinterpret_cast<const char *>(&SectionLen), 1);
  File.write(reinterpret_cast<const char *>(&NameLen), 1);
  File.write(reinterpret_cast<const char *>(&DEFAULT_CUSOTM_SECTION_NAME),
             NameLen);
  File.write(reinterpret_cast<const char *>(&Signature[0]), SectionLen);
  return {};
}

Expect<bool> Signature::verify(const Span<Byte> CustomSec,
                               std::filesystem::path Path,
                               const std::filesystem::path &PubKeyPath) {
  /// Return void for this template
  std::ifstream PubKeyFile(PubKeyPath.string(), std::ios::binary);
  std::ifstream SourceFile(Path.string(), std::ios::binary);
  Span<const Byte> Signature, PublicKey;
  try {
    PubKeyFile.exceptions(PubKeyFile.failbit);
  } catch (const std::ios_base::failure &Error) {
    // Failure handling
  }
  return Alg.verify(CustomSec, Signature, PublicKey);
}

// Expect<Span<Byte>> keygen(Span<const uint8_t> Code) {
Expect<std::vector<Byte>> Signature::keygen(const Span<uint8_t> Code,
                                            const std::filesystem::path &Path) {
  return Alg.keygen(Code, Path);
}

} // namespace Signature
} // namespace WasmEdge