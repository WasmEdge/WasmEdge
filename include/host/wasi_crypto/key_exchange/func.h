// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Kx {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(WasiCryptoContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WasiCryptoContext &Ctx;
};

class Dh : public HostFunction<Dh> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_publickey_t Pk, __wasi_secretkey_t Sk,
                        uint8_t_ptr /* Out */ ArrayOutputPtr);
};

class Encapsulate : public HostFunction<Encapsulate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_publickey_t Pk,
                        uint8_t_ptr /* Out */ ArrayOutput1Ptr,
                        uint8_t_ptr /* Out */ ArrayOutput2Ptr);
};

class Decapsulate : public HostFunction<Decapsulate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secretkey_t Sk,
                        const_uint8_t_ptr EncapsulatedSecret,
                        __wasi_size_t EncapsulatedSecretLen,
                        uint8_t_ptr /* Out */ ArrayOutputPtr);
};

} // namespace Kx
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
