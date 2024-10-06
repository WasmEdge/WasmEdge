// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "utils/evp_wrapper.h"
#include "utils/error.h"

#include <openssl/bn.h>
#include <openssl/ec.h>

#include <limits>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {

EVP_PKEY *pemReadPUBKEY(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  if (size_t Size;
      BIO_write_ex(Bio.get(), Encoded.data(), Encoded.size(), &Size)) {
    if (Size != Encoded.size()) {
      return nullptr;
    }
  } else {
    return nullptr;
  }

  return PEM_read_bio_PUBKEY(Bio.get(), nullptr, nullptr, nullptr);
}

WasiCryptoExpect<std::vector<uint8_t>> pemWritePUBKEY(EVP_PKEY *Key) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslCheck(PEM_write_bio_PUBKEY(Bio.get(), Key));

  BUF_MEM *Mem = nullptr;
  opensslCheck(BIO_get_mem_ptr(Bio.get(), &Mem));
  std::vector<uint8_t> Ret(Mem->length);

  if (size_t Size; BIO_read_ex(Bio.get(), Ret.data(), Ret.size(), &Size)) {
    ensureOrReturn(Size == Ret.size(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  return Ret;
}

EVP_PKEY *pemReadPrivateKey(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  if (size_t Size;
      BIO_write_ex(Bio.get(), Encoded.data(), Encoded.size(), &Size)) {
    if (Size != Encoded.size()) {
      return nullptr;
    }
  } else {
    return nullptr;
  }

  return PEM_read_bio_PrivateKey(Bio.get(), nullptr, nullptr, nullptr);
}

WasiCryptoExpect<SecretVec> pemWritePrivateKey(EVP_PKEY *Key) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslCheck(PEM_write_bio_PrivateKey(Bio.get(), Key, nullptr, nullptr, 0,
                                        nullptr, nullptr));

  BUF_MEM *Mem = nullptr;
  opensslCheck(BIO_get_mem_ptr(Bio.get(), &Mem));
  SecretVec Ret(Mem->length);

  if (size_t Size; BIO_read_ex(Bio.get(), Ret.data(), Ret.size(), &Size)) {
    ensureOrReturn(Size == Ret.size(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  return Ret;
}

EVP_PKEY *d2iPUBKEY(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  if (size_t Size;
      BIO_write_ex(Bio.get(), Encoded.data(), Encoded.size(), &Size)) {
    if (Size != Encoded.size()) {
      return nullptr;
    }
  } else {
    return nullptr;
  }

  return d2i_PUBKEY_bio(Bio.get(), nullptr);
}

WasiCryptoExpect<std::vector<uint8_t>> i2dPUBKEY(EVP_PKEY *Key) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PUBKEY_bio(Bio.get(), Key));

  BUF_MEM *Mem = nullptr;
  opensslCheck(BIO_get_mem_ptr(Bio.get(), &Mem));
  std::vector<uint8_t> Ret(Mem->length);

  if (size_t Size; BIO_read_ex(Bio.get(), Ret.data(), Ret.size(), &Size)) {
    ensureOrReturn(Size == Ret.size(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  return Ret;
}

EVP_PKEY *d2iPrivateKey(Span<const uint8_t> Encoded) {
  BioPtr Bio{BIO_new(BIO_s_mem())};

  if (size_t Size;
      BIO_write_ex(Bio.get(), Encoded.data(), Encoded.size(), &Size)) {
    if (Size != Encoded.size()) {
      return nullptr;
    }
  } else {
    return nullptr;
  }

  return d2i_PrivateKey_bio(Bio.get(), nullptr);
}

WasiCryptoExpect<SecretVec> i2dPrivateKey(EVP_PKEY *Key) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PrivateKey_bio(Bio.get(), Key));

  BUF_MEM *Mem = nullptr;
  opensslCheck(BIO_get_mem_ptr(Bio.get(), &Mem));
  SecretVec Ret(Mem->length);

  if (size_t Size; BIO_read_ex(Bio.get(), Ret.data(), Ret.size(), &Size)) {
    ensureOrReturn(Size == Ret.size(), __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  } else {
    return WasiCryptoUnexpect(__WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);
  }

  return Ret;
}

ECDSA_SIG *d2iEcdsaSig(Span<const uint8_t> Encoded) {
  if (Encoded.size() > static_cast<size_t>(std::numeric_limits<long>::max())) {
    return nullptr;
  }
  auto *Data = Encoded.data();
  return d2i_ECDSA_SIG(nullptr, &Data, static_cast<long>(Encoded.size()));
}

WasiCryptoExpect<std::vector<uint8_t>> i2dEcdsaSig(ECDSA_SIG *Sig) {
  int SigSize = i2d_ECDSA_SIG(Sig, nullptr);
  ensureOrReturn(SigSize >= 0, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  std::vector<uint8_t> Res(static_cast<size_t>(SigSize));

  auto *Data = Res.data();
  auto NewSize = i2d_ECDSA_SIG(Sig, &Data);
  ensureOrReturn(NewSize == SigSize, __WASI_CRYPTO_ERRNO_ALGORITHM_FAILURE);

  return Res;
}

ECDSA_SIG *o2iEcdsaSig(Span<const uint8_t> Encoded) {
  if (Encoded.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
    return nullptr;
  }
  int EncodedSize = static_cast<int>(Encoded.size());
  BnPtr R{BN_bin2bn(Encoded.data(), EncodedSize / 2, nullptr)};
  BnPtr S{
      BN_bin2bn(Encoded.data() + EncodedSize / 2, EncodedSize / 2, nullptr)};
  EcdsaSigPtr Sig{ECDSA_SIG_new()};
  if (!ECDSA_SIG_set0(Sig.get(), R.release(), S.release())) {
    return nullptr;
  }

  return Sig.release();
}

WasiCryptoExpect<std::vector<uint8_t>> i2oEcdsaSig(ECDSA_SIG *Sig) {
  auto *R = ECDSA_SIG_get0_r(Sig);
  auto *S = ECDSA_SIG_get0_s(Sig);
  auto RSize = static_cast<size_t>(BN_num_bytes(R));
  auto SSize = static_cast<size_t>(BN_num_bytes(S));
  std::vector<uint8_t> Res(RSize + SSize);
  opensslCheck(BN_bn2bin(R, Res.data()));
  opensslCheck(BN_bn2bin(S, Res.data() + RSize));

  return Res;
}

SharedEvpPkey::~SharedEvpPkey() noexcept {
  if (Pkey != nullptr) {
    EVP_PKEY_free(Pkey);
    Pkey = nullptr;
  }
}

SharedEvpPkey::SharedEvpPkey(const SharedEvpPkey &Rhs) noexcept
    : Pkey(Rhs.Pkey) {
  if (Rhs.Pkey != nullptr) {
    EVP_PKEY_up_ref(Pkey);
  }
}

SharedEvpPkey::SharedEvpPkey(SharedEvpPkey &&Rhs) noexcept : Pkey(Rhs.Pkey) {
  Rhs.Pkey = nullptr;
}

EVP_PKEY *SharedEvpPkey::get() const noexcept { return Pkey; }

SharedEvpPkey::operator bool() const noexcept { return Pkey != nullptr; }

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
