// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC
#include "host/wasi_crypto/utils/evp_wrapper.h"

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

WasiCryptoExpect<std::vector<uint8_t>> pemWritePrivateKey(EVP_PKEY *Key) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslCheck(PEM_write_bio_PrivateKey(Bio.get(), Key, nullptr, nullptr, 0,
                                        nullptr, nullptr));

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

WasiCryptoExpect<std::vector<uint8_t>> i2dPrivateKey(EVP_PKEY *Key) {
  BioPtr Bio{BIO_new(BIO_s_mem())};
  opensslCheck(i2d_PrivateKey_bio(Bio.get(), Key));

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

} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
