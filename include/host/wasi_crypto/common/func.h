// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(WasiCryptoContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WasiCryptoContext &Ctx;
};

class ArrayOutputLen : public HostFunction<ArrayOutputLen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_array_output_t ArrayOutputHandle,
                        uint32_t /* Out */ SizePtr);
};

class ArrayOutputPull : public HostFunction<ArrayOutputPull> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_array_output_t ArrayOutputHandle,
                        uint8_t_ptr BufPtr, __wasi_size_t BufLen,
                        uint32_t /* Out */ SizePtr);
};

class OptionsOpen : public HostFunction<OptionsOpen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType, uint32_t /* Out */ OptionsPtr);
};

class OptionsClose : public HostFunction<OptionsClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle);
};

class OptionsSet : public HostFunction<OptionsSet> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, const_uint8_t_ptr ValuePtr,
                        __wasi_size_t ValueLen);
};

class OptionsSetU64 : public HostFunction<OptionsSetU64> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint64_t Value);
};

class OptionsSetGuestBuffer : public HostFunction<OptionsSetGuestBuffer> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint8_t_ptr BufPtr,
                        __wasi_size_t BufLen);
};

class SecretsMangerOpen : public HostFunction<SecretsMangerOpen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t OptOptionsPtr,
                        uint32_t /* Out */ SecretsManagerPtr);
};

class SecretsMangerClose : public HostFunction<SecretsMangerClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager);
};

class SecretsMangerInvalidate : public HostFunction<SecretsMangerInvalidate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        const_uint8_t_ptr KeyIdPtr, __wasi_size_t KeyIdLen,
                        __wasi_version_t Version);
};

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
