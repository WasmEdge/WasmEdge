// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/symmetric/aeads/aeads_state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

namespace {
/// TODO: Can replace it with a builtin function.
/// avoid overflow
constexpr bool checkAdd(Span<const uint8_t> Data, __wasi_size_t TagSize) {
  return (SIZE_MAX - TagSize) < Data.size();
}

/// avoid overflow
constexpr bool checkSub(Span<const uint8_t> Data, __wasi_size_t TagSize) {
  return TagSize > Data.size();
}
} // namespace

WasiCryptoExpect<__wasi_size_t> AEADsState::encrypt(Span<uint8_t> Out,
                                                    Span<const uint8_t> Data) {
  auto TagSize = maxTagLen();
  if (!TagSize) {
    return WasiCryptoUnexpect(TagSize);
  }

  if (checkAdd(Data, *TagSize)) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  ensureOrReturn(Out.size() == Data.size() + *TagSize,
                 __WASI_CRYPTO_ERRNO_INVALID_LENGTH);

  return encryptUnchecked(Out, Data);
}

WasiCryptoExpect<Tag> AEADsState::encryptDetached(Span<uint8_t> Out,
                                                  Span<const uint8_t> Data) {
  if (Out.size() != Data.size())
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  return encryptDetachedUnchecked(Out, Data);
}

WasiCryptoExpect<__wasi_size_t> AEADsState::decrypt(Span<uint8_t> Out,
                                                    Span<const uint8_t> Data) {
  if (auto TagSize = maxTagLen(); !TagSize) {
    return WasiCryptoUnexpect(TagSize);
  } else {
    if (checkSub(Data, *TagSize))
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
    if (Out.size() != Data.size() - *TagSize)
      return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_OVERFLOW);
  }

  auto Result = decryptUnchecked(Out, Data);
  if (!Result) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(Result);
  }

  return *Result;
}

WasiCryptoExpect<__wasi_size_t>
AEADsState::decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                            Span<uint8_t> RawTag) {
  if (Out.size() != Data.size()) {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_INVALID_LENGTH);
  }

  auto Result = decryptDetachedUnchecked(Out, Data, RawTag);
  if (!Result) {
    std::fill(Out.begin(), Out.end(), 0);
    return WasiCryptoUnexpect(Result);
  }

  return *Result;
}

WasiCryptoExpect<__wasi_size_t>
AEADsState::encryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data) {
  auto Tag = encryptDetached(Out.first(Data.size()), Data);
  if (!Tag) {
    return WasiCryptoUnexpect(Tag);
  }

  std::copy(Tag->data().cbegin(), Tag->data().cend(),
            Out.subspan(Data.size()).begin());

  return Out.size();
}

WasiCryptoExpect<__wasi_size_t>
AEADsState::decryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data) {
  return decryptDetachedUnchecked(Out, Data.first(Out.size()),
                                  Data.subspan(Out.size()));
}

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
