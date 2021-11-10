// SPDX-License-Identifier: Apache-2.0

#include "host/wasi_crypto/common/func.h"
#include "host/wasi_crypto/util.h"
#include "common/errcode.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {

namespace {
template <typename T> struct WasiRawType {
  using Type = std::underlying_type_t<T>;
};
template <> struct WasiRawType<uint8_t> { using Type = uint8_t; };
template <> struct WasiRawType<uint16_t> { using Type = uint16_t; };
template <> struct WasiRawType<uint32_t> { using Type = uint32_t; };
template <> struct WasiRawType<uint64_t> { using Type = uint64_t; };

template <typename T> using WasiRawTypeT = typename WasiRawType<T>::Type;

template <typename T> WASICrypto::WasiCryptoExpect<T> cast(uint64_t) noexcept;

template <>
WASICrypto::WasiCryptoExpect<__wasi_algorithm_type_e_t>
cast(uint64_t Algorithm) noexcept {
  switch (static_cast<WasiRawTypeT<__wasi_algorithm_type_e_t>>(Algorithm)) {
  case __WASI_ALGORITHM_TYPE_SIGNATURES:
  case __WASI_ALGORITHM_TYPE_SYMMETRIC:
  case __WASI_ALGORITHM_TYPE_KEY_EXCHANGE:
    return static_cast<__wasi_algorithm_type_e_t>(Algorithm);
  default:
    return WASICrypto::WasiCryptoUnexpect(
        __WASI_CRYPTO_ERRNO_INVALID_OPERATION);
  }
}

} // namespace

Expect<uint32_t>
WasiCryptoCommonArrayOutputLen::body(Runtime::Instance::MemoryInstance *MemInst,
                                     __wasi_array_output_t ArrayOutputHandle,
                                     uint32_t /* Out */ SizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Len = Ctx.arrayOutputLen(ArrayOutputHandle);
  if (unlikely(!Len)) {
    return Len.error();
  }

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  *Size = *Len;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiCryptoCommonArrayOutputPull::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_array_output_t ArrayOutputHandle, uint8_t_ptr BufPtr,
    __wasi_size_t BufLen, uint32_t /* Out */ SizePtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *BufMem = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);

  if (unlikely(BufMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  Span<uint8_t> Buf{BufMem, BufLen};

  auto Res = Ctx.arrayOutputPull(ArrayOutputHandle, Buf);
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const Size = MemInst->getPointer<__wasi_size_t *>(SizePtr);
  if (unlikely(Size == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  *Size = *Res;

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiCryptoCommonOptionsOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                  uint32_t AlgorithmType,
                                  uint32_t /* Out */ OptionsPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  __wasi_algorithm_type_e_t Algorithm;
  if (auto Res = cast<__wasi_algorithm_type_e_t>(AlgorithmType);
      unlikely(!Res)) {
    return Res.error();
  } else {
    Algorithm = *Res;
  }

  auto *const Options = MemInst->getPointer<__wasi_options_t *>(OptionsPtr);
  if (unlikely(Options == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  if (auto Res = Ctx.optionsOpen(Algorithm); unlikely(!Res)) {
    return Res.error();
  } else {
    *Options = *Res;
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiCryptoCommonOptionsClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_options_t Handle) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.optionsClose(Handle);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiCryptoCommonOptionsSet::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_options_t Handle,
    const_uint8_t_ptr NamePtr, __wasi_size_t NameLen,
    const_uint8_t_ptr ValuePtr, __wasi_size_t ValueLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *ValueMem = MemInst->getPointer<uint8_t const *>(ValuePtr, ValueLen);
  if (unlikely(ValueMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }
  Span<uint8_t const> Value{ValueMem, ValueLen};
  auto *const NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);

  if (unlikely(ValueMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.optionsSet(Handle, {NameMem, NameLen}, Value);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiCryptoCommonOptionsSetU64::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_options_t Handle,
    const_uint8_t_ptr NamePtr, __wasi_size_t NameLen, uint64_t Value) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);
  if (unlikely(NameMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.optionsSetU64(Handle, {NameMem, NameLen}, Value);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiCryptoCommonOptionsSetGuestBuffer::body(
    Runtime::Instance::MemoryInstance *MemInst, __wasi_options_t Handle,
    const_uint8_t_ptr NamePtr, __wasi_size_t NameLen, uint8_t_ptr BufPtr,
    __wasi_size_t BufLen) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const NameMem = MemInst->getPointer<const char *>(NamePtr, NameLen);

  if(unlikely(NameMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *BufMem = MemInst->getPointer<uint8_t *>(BufPtr, BufLen);

  if (unlikely(BufMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  Span<uint8_t> Buf{BufMem, BufLen};

  auto Res = Ctx.optionsSetGuestBuffer(Handle, {NameMem, NameLen}, Buf);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiCryptoSecretsMangerOpen::body(Runtime::Instance::MemoryInstance *MemInst,
                                  uint32_t OptOptionsPtr,
                                  uint32_t /* Out */ SecretsManagerPtr) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const OptOption =
      MemInst->getPointer<__wasi_opt_options_t *>(OptOptionsPtr);

  if (unlikely(OptOption == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.secretsMangerOpen(WASICrypto::parseCUnion<__wasi_options_t>(*OptOption));
  if (unlikely(!Res)) {
    return Res.error();
  }

  auto *const SecretsManager =
      MemInst->getPointer<__wasi_secrets_manager_t *>(SecretsManagerPtr);
  if (unlikely(SecretsManager == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  *SecretsManager = *Res;
  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t>
WasiCryptoSecretsMangerClose::body(Runtime::Instance::MemoryInstance *MemInst,
                                   __wasi_secrets_manager_t SecretsManger) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto Res = Ctx.secretsMangerClose(SecretsManger);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

Expect<uint32_t> WasiCryptoSecretsMangerInvalidate::body(
    Runtime::Instance::MemoryInstance *MemInst,
    __wasi_secrets_manager_t SecretsManger, const_uint8_t_ptr KeyIdPtr,
    __wasi_size_t KeyIdLen, __wasi_version_t Version) {
  /// Check memory instance from module.
  if (MemInst == nullptr) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  auto *const KeyIdMem =
      MemInst->getPointer<uint8_t const *>(KeyIdPtr, KeyIdLen);

  if (unlikely(KeyIdMem == nullptr)) {
    return __WASI_CRYPTO_ERRNO_INTERNAL_ERROR;
  }

  Span<uint8_t const> KeyId{KeyIdMem, KeyIdLen};

  auto Res = Ctx.secretsManagerInvalidate(SecretsManger, KeyId, Version);
  if (unlikely(!Res)) {
    return Res.error();
  }

  return __WASI_CRYPTO_ERRNO_SUCCESS;
}

} // namespace Host
} // namespace WasmEdge
