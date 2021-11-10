// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/ctx.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiCryptoCommon : public Runtime::HostFunction<T> {
public:
  WasiCryptoCommon(WASICrypto::CommonContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WASICrypto::CommonContext &Ctx;
};

class WasiCryptoCommonArrayOutputLen
    : public WasiCryptoCommon<WasiCryptoCommonArrayOutputLen> {
public:
  WasiCryptoCommonArrayOutputLen(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_array_output_t ArrayOutputHandle,
                        uint32_t /* Out */ SizePtr);
};

class WasiCryptoCommonArrayOutputPull
    : public WasiCryptoCommon<WasiCryptoCommonArrayOutputPull> {
public:
  WasiCryptoCommonArrayOutputPull(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_array_output_t ArrayOutputHandle,
                        uint8_t_ptr BufPtr, __wasi_size_t BufLen,
                        uint32_t /* Out */ SizePtr);
};

class WasiCryptoCommonOptionsOpen
    : public WasiCryptoCommon<WasiCryptoCommonOptionsOpen> {
public:
  WasiCryptoCommonOptionsOpen(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType,
                        uint32_t /* Out */ OptionsPtr);
};

class WasiCryptoCommonOptionsClose
    : public WasiCryptoCommon<WasiCryptoCommonOptionsClose> {
public:
  WasiCryptoCommonOptionsClose(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle);
};

class WasiCryptoCommonOptionsSet
    : public WasiCryptoCommon<WasiCryptoCommonOptionsSet> {
public:
  WasiCryptoCommonOptionsSet(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, const_uint8_t_ptr ValuePtr,
                        __wasi_size_t ValueLen);
};

class WasiCryptoCommonOptionsSetU64
    : public WasiCryptoCommon<WasiCryptoCommonOptionsSetU64> {
public:
  WasiCryptoCommonOptionsSetU64(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint64_t Value);
};

class WasiCryptoCommonOptionsSetGuestBuffer
    : public WasiCryptoCommon<WasiCryptoCommonOptionsSetGuestBuffer> {
public:
  WasiCryptoCommonOptionsSetGuestBuffer(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint8_t_ptr BufPtr,
                        __wasi_size_t BufLen);
};

class WasiCryptoSecretsMangerOpen
    : public WasiCryptoCommon<WasiCryptoSecretsMangerOpen> {
public:
  WasiCryptoSecretsMangerOpen(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t OptOptionsPtr,
                        uint32_t /* Out */ SecretsManagerPtr);
};

class WasiCryptoSecretsMangerClose
    : public WasiCryptoCommon<WasiCryptoSecretsMangerClose> {
public:
  WasiCryptoSecretsMangerClose(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager);
};

class WasiCryptoSecretsMangerInvalidate
    : public WasiCryptoCommon<WasiCryptoSecretsMangerInvalidate> {
public:
  WasiCryptoSecretsMangerInvalidate(WASICrypto::CommonContext &HostCtx)
      : WasiCryptoCommon(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        const_uint8_t_ptr KeyIdPtr, __wasi_size_t KeyIdLen,
                        __wasi_version_t Version);
};

} // namespace Host
} // namespace WasmEdge
