// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/symmetric/state.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

/// AEADs MUST support the following operations:
///
/// absorb()
/// max_tag_len()
/// encrypt()
/// encrypt_detached()
/// decrypt()
/// decrypt_detached()
///
/// Notice: encrypt()/decrypt() can delegate by
/// encrypt_detached()/decrypt_detached()
class AEADsState : public State {
public:
  /// Absorbs additional data. Multiple calls to absorb() MUST be equivalent to
  /// a single call with a concatenation of the inputs.
  ///
  /// @param Data Additional data
  /// @return Nothing or WasiCrypto error
  virtual WasiCryptoExpect<void> absorb(Span<const uint8_t> Data) override = 0;

  /// Return the length required to encode the authentication tag
  /// and optional padding bytes. The returned length MUST be constant for a
  /// given algorithm. Guest applications are expected to provide an output
  /// buffer whose size is the size of the message, plus the max_tag_len()
  /// output value.
  ///
  /// @return the length required to encode the authentication tag
  /// and optional padding bytes.
  virtual WasiCryptoExpect<__wasi_size_t> maxTagLen() override = 0;

  /// Check Out.size() == Data.size() + maxTagLen(), then call
  /// encryptUnchecked(Out, Data) or return error if not equal
  ///
  /// @param Out The encrypted data text.
  /// @param Data The data to be encrypted
  /// @return Tag's size or
  /// `__WASI_CRYPTO_ERRNO_OVERFLOW`/`__WASI_CRYPTO_ERRNO_INVALID_LENGTH`
  WasiCryptoExpect<__wasi_size_t>
  encrypt(Span<uint8_t> Out, Span<const uint8_t> Data) override final;

  /// Check Out.size() == Data.size(), then call
  /// encryptDetachedUnchecked(Out, Data) or error if not equal
  ///
  /// @param Out The encrypted data text.
  /// @param Data The data to be encrypted
  /// @return Tag
  /// or `__WASI_CRYPTO_ERRNO_OVERFLOW`/`__WASI_CRYPTO_ERRNO_INVALID_LENGTH`
  WasiCryptoExpect<Tag>
  encryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data) override final;

  /// Check Out.size() = Data.size() + maxTagLen(), then call
  /// decryptDetachedUnchecked(Out, Data) or error if not equal
  ///
  /// @param Out
  /// @param Data
  /// @return Size or
  /// `__WASI_CRYPTO_ERRNO_OVERFLOW`/`__WASI_CRYPTO_ERRNO_INVALID_LENGTH`
  WasiCryptoExpect<__wasi_size_t>
  decrypt(Span<uint8_t> Out, Span<const uint8_t> Data) override final;

  /// Guarantee the Out.size() = Data.size()
  WasiCryptoExpect<__wasi_size_t>
  decryptDetached(Span<uint8_t> Out, Span<const uint8_t> Data,
                  Span<uint8_t> RawTag) override final;

protected:
  /// delegate by encryptDetachedUnchecked()
  WasiCryptoExpect<__wasi_size_t> encryptUnchecked(Span<uint8_t> Out,
                                                   Span<const uint8_t> Data);

  /// delegate by decryptDetachedUnchecked()
  WasiCryptoExpect<__wasi_size_t> decryptUnchecked(Span<uint8_t> Out,
                                                   Span<uint8_t const> Data);

  // It guarantee the Out.size() = Data.size()
  virtual WasiCryptoExpect<Tag>
  encryptDetachedUnchecked(Span<uint8_t> Out, Span<uint8_t const> Data) = 0;

  /// Decryption functions MUST check the authentication tag, and SHOULD NOT
  /// leave decrypted data in the output buffer if the authentication tag didn't
  /// verify. If this is the case, they SHOULD zero the entire output buffer and
  /// MUST return an invalid_tag error code.
  virtual WasiCryptoExpect<__wasi_size_t>
  decryptDetachedUnchecked(Span<uint8_t> Out, Span<uint8_t const> Data,
                           Span<uint8_t const> RawTag) = 0;

private:
  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) override final {
    return State::squeeze(Out);
  }
  WasiCryptoExpect<void> ratchet() override final { return State::ratchet(); }

  WasiCryptoExpect<std::unique_ptr<Key>>
  squeezeKey(SymmetricAlgorithm KeyAlg) override final {
    return State::squeezeKey(KeyAlg);
  }
  WasiCryptoExpect<Tag> squeezeTag() override final {
    return State::squeezeTag();
  }
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
