#include "helper.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

std::vector<uint8_t> operator"" _u8(const char *Str, std::size_t Len) noexcept {
  return std::vector<uint8_t>{reinterpret_cast<uint8_t const *>(Str),
                              reinterpret_cast<uint8_t const *>(Str) + Len};
}

std::vector<uint8_t> operator"" _u8v(const char *Str,
                                     std::size_t Len) noexcept {
  std::vector<uint8_t> Res(Len / 2);
  for (size_t I = 0; I < Len; I += 2) {
    std::string Tran{Str + I, 2};
    Res[I / 2] = static_cast<uint8_t>(std::strtol(Tran.c_str(), nullptr, 16));
  }
  return Res;
}

void WasiCryptoTest::writeDummyMemoryContent() noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(0), 64, UINT8_C(0xa5));
}

void WasiCryptoTest::writeString(std::string_view String, uint32_t Ptr) {
  std::copy(String.begin(), String.end(), MemInst.getPointer<uint8_t *>(Ptr));
}

void WasiCryptoTest::writeSpan(Span<const uint8_t> Content, uint32_t Ptr) {
  std::copy(Content.begin(), Content.end(), MemInst.getPointer<uint8_t *>(Ptr));
}

void WasiCryptoTest::writeOptKey(std::optional<uint32_t> OptKey, uint32_t Ptr) {
  __wasi_opt_symmetric_key_t Key;
  if (OptKey) {
    Key.tag = __WASI_OPT_SYMMETRIC_KEY_U_SOME;
    Key.u = {*OptKey};
  } else {
    Key.tag = __WASI_OPT_SYMMETRIC_KEY_U_NONE;
  }
  auto *BeginPlace = MemInst.getPointer<__wasi_opt_symmetric_key_t *>(Ptr);
  *BeginPlace = Key;
}

void WasiCryptoTest::writeOptOptions(std::optional<__wasi_options_t> OptOptions,
                                     uint32_t Ptr) {
  __wasi_opt_options_t Options;
  if (OptOptions) {
    Options.tag = __WASI_OPT_OPTIONS_U_SOME;
    Options.u = {*OptOptions};
  } else {
    Options.tag = __WASI_OPT_OPTIONS_U_NONE;
  }
  auto *BeginPlace = MemInst.getPointer<__wasi_opt_options_t *>(Ptr);
  *BeginPlace = Options;
}
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
