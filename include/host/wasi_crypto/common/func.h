// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/common/ctx.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Common {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(WASICrypto::CommonContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WASICrypto::CommonContext &Ctx;
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
  OptionsOpen(WASICrypto::CommonContext &HostCtx) : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t AlgorithmType, uint32_t /* Out */ OptionsPtr);
};

class OptionsClose : public HostFunction<OptionsClose> {
public:
  OptionsClose(WASICrypto::CommonContext &HostCtx) : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle);
};

class OptionsSet : public HostFunction<OptionsSet> {
public:
  OptionsSet(WASICrypto::CommonContext &HostCtx) : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, const_uint8_t_ptr ValuePtr,
                        __wasi_size_t ValueLen);
};

class OptionsSetU64 : public HostFunction<OptionsSetU64> {
public:
  OptionsSetU64(WASICrypto::CommonContext &HostCtx) : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint64_t Value);
};

class OptionsSetGuestBuffer : public HostFunction<OptionsSetGuestBuffer> {
public:
  OptionsSetGuestBuffer(WASICrypto::CommonContext &HostCtx)
      : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_options_t Handle, const_uint8_t_ptr NamePtr,
                        __wasi_size_t NameLen, uint8_t_ptr BufPtr,
                        __wasi_size_t BufLen);
};

class SecretsMangerOpen : public HostFunction<SecretsMangerOpen> {
public:
  SecretsMangerOpen(WASICrypto::CommonContext &HostCtx)
      : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t OptOptionsPtr,
                        uint32_t /* Out */ SecretsManagerPtr);
};

class SecretsMangerClose : public HostFunction<SecretsMangerClose> {
public:
  SecretsMangerClose(WASICrypto::CommonContext &HostCtx)
      : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager);
};

class SecretsMangerInvalidate : public HostFunction<SecretsMangerInvalidate> {
public:
  SecretsMangerInvalidate(WASICrypto::CommonContext &HostCtx)
      : HostFunction(HostCtx) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        const_uint8_t_ptr KeyIdPtr, __wasi_size_t KeyIdLen,
                        __wasi_version_t Version);
};

} // namespace Common
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
