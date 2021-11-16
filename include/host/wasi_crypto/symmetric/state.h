// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/options.h"
#include "host/wasi_crypto/lock.h"
#include "host/wasi_crypto/symmetric/alg.h"
#include "host/wasi_crypto/symmetric/key.h"
#include "host/wasi_crypto/symmetric/options.h"
#include "host/wasi_crypto/symmetric/tag.h"
#include "wasi_crypto/api.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace WasmEdge {
namespace Host {
namespace WASICrypto {

class SymmetricStateBase {
public:
  virtual ~SymmetricStateBase() = default;

  virtual WasiCryptoExpect<std::vector<uint8_t>>
  optionsGet(std::string_view Name) = 0;

  virtual WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) = 0;

  virtual WasiCryptoExpect<void> absorb(Span<uint8_t const> Data);

  /// The output buffer given to the squeeze function can be smaller than the
  /// hash function output size. In that case, the implementation MUST truncate
  /// the output to the requested length.
  ///
  /// If the requested size exceeds what the hash function can output, the
  /// invalid_length error code MUST be returned.
  virtual WasiCryptoExpect<void> squeeze(Span<uint8_t> Out);

  virtual WasiCryptoExpect<void> ratchet();

  /// @param Out The encrypted data text.
  /// @param Data The data to be encrypted
  /// @return The data
  /// Must guarantee the @param Out.size() = @param Data.size() +
  /// maxTagLen(), then call encryptUnchecked(Out, Data)
  WasiCryptoExpect<__wasi_size_t> encrypt(Span<uint8_t> Out,
                                          Span<uint8_t const> Data);

  /// @param Out The encrypted data text.
  /// @param Data The data to be encrypted
  /// @return Tag
  /// Must guarantee the @param Out.size() = @param Data.size(),
  /// then call encryptDetachedUnchecked(Out, Data)
  WasiCryptoExpect<SymmetricTag> encryptDetached(Span<uint8_t> Out,
                                                 Span<uint8_t const> Data);

  WasiCryptoExpect<__wasi_size_t> decrypt(Span<uint8_t> Out,
                                          Span<uint8_t const> Data);

  WasiCryptoExpect<__wasi_size_t> decryptDetached(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data,
                                                  Span<uint8_t> RawTag);

  virtual WasiCryptoExpect<SymmetricKey> squeezeKey(SymmetricAlgorithm Alg);

  virtual WasiCryptoExpect<SymmetricTag> squeezeTag();

  virtual WasiCryptoExpect<__wasi_size_t> maxTagLen();

protected:
  SymmetricStateBase(SymmetricAlgorithm Alg);

  virtual WasiCryptoExpect<__wasi_size_t>
  encryptUnchecked(Span<uint8_t> Out, Span<const uint8_t> Data);

  // It guarantee the Out.size() = Data.size()
  virtual WasiCryptoExpect<SymmetricTag>
  encryptDetachedUnchecked(Span<uint8_t> Out, Span<uint8_t const> Data);

  // It guarantee the Out.size() = Data.size() + Tag.size()
  virtual WasiCryptoExpect<__wasi_size_t>
  decryptUnchecked(Span<uint8_t> Out, Span<uint8_t const> Data);

  // It guarantee the Out.size() = Data.size()
  virtual WasiCryptoExpect<__wasi_size_t>
  decryptDetachedUnchecked(Span<uint8_t> Out, Span<uint8_t const> Data,
                           Span<uint8_t const> RawTag);
  SymmetricAlgorithm Alg;
};

class SymmetricState {
public:
  SymmetricState(std::unique_ptr<SymmetricStateBase> Inner);

  static WasiCryptoExpect<SymmetricState>
  make(SymmetricAlgorithm Alg, std::optional<SymmetricKey> OptKey,
       std::optional<SymmetricOptions> OptOptions);

  WasiCryptoExpect<std::vector<uint8_t>> optionsGet(std::string_view Name) {
    return Inner->locked([&Name](std::unique_ptr<SymmetricStateBase> &State) {
      return State->optionsGet(Name);
    });
  }

  WasiCryptoExpect<uint64_t> optionsGetU64(std::string_view Name) {
    return Inner->locked([&Name](std::unique_ptr<SymmetricStateBase> &State) {
      return State->optionsGetU64(Name);
    });
  }

  WasiCryptoExpect<void> absorb(Span<uint8_t const> Data) {
    return Inner->locked([&Data](std::unique_ptr<SymmetricStateBase> &State) {
      return State->absorb(Data);
    });
  }

  WasiCryptoExpect<void> squeeze(Span<uint8_t> Out) {
    return Inner->locked([&Out](std::unique_ptr<SymmetricStateBase> &State) {
      return State->squeeze(Out);
    });
  }

  WasiCryptoExpect<void> ratchet() {
    return Inner->locked([](std::unique_ptr<SymmetricStateBase> &State) {
      return State->ratchet();
    });
  }

  WasiCryptoExpect<__wasi_size_t> encrypt(Span<uint8_t> Out,
                                          Span<uint8_t const> Data) {
    return Inner->locked(
        [&Out, &Data](std::unique_ptr<SymmetricStateBase> &State) {
          return State->encrypt(Out, Data);
        });
  }

  WasiCryptoExpect<SymmetricTag> encryptDetached(Span<uint8_t> Out,
                                                 Span<uint8_t const> Data) {

    return Inner->locked(
        [&Out, &Data](std::unique_ptr<SymmetricStateBase> &State) {
          return State->encryptDetached(Out, Data);
        });
  }

  WasiCryptoExpect<__wasi_size_t> decrypt(Span<uint8_t> Out,
                                          Span<uint8_t const> Data) {
    return Inner->locked(
        [&Out, &Data](std::unique_ptr<SymmetricStateBase> &State) {
          return State->decrypt(Out, Data);
        });
  }

  WasiCryptoExpect<__wasi_size_t> decryptDetached(Span<uint8_t> Out,
                                                  Span<uint8_t const> Data,
                                                  Span<uint8_t> RawTag) {
    return Inner->locked(
        [&Out, &Data, &RawTag](std::unique_ptr<SymmetricStateBase> &State) {
          return State->decryptDetached(Out, Data, RawTag);
        });
  }

  WasiCryptoExpect<SymmetricKey> squeezeKey(SymmetricAlgorithm Alg) {
    return Inner->locked([Alg](std::unique_ptr<SymmetricStateBase> &State) {
      return State->squeezeKey(Alg);
    });
  }

  WasiCryptoExpect<SymmetricTag> squeezeTag() {
    return Inner->locked([](std::unique_ptr<SymmetricStateBase> &State) {
      return State->squeezeTag();
    });
  }

  WasiCryptoExpect<__wasi_size_t> maxTagLen() {
    return Inner->locked([](std::unique_ptr<SymmetricStateBase> &State) {
      return State->maxTagLen();
    });
  }

private:
  std::shared_ptr<Mutex<std::unique_ptr<SymmetricStateBase>>> Inner;
};

} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
