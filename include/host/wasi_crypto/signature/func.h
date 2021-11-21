// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi_crypto/ctx.h"
#include "runtime/hostfunc.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Signatures {
template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(WasiCryptoContext &Ctx)
      : Runtime::HostFunction<T>(0), Ctx(Ctx) {}

protected:
  WasiCryptoContext &Ctx;
};

class Export : public HostFunction<Export> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_t Signature, uint16_t Encoding,
                        uint32_t /* Out */ ArrayOutputPtr);
};

class Import : public HostFunction<Import> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgorithmPtr,
                        __wasi_size_t AlgorithmLen,
                        const_uint8_t_ptr EncodedPtr, __wasi_size_t EncodedLen,
                        uint16_t Encoding, uint32_t /* Out */ SignaturePtr);
};

class StateOpen : public HostFunction<StateOpen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_keypair_t Kp,
                        uint32_t /* Out */ SignatureStatePtr);
};

class StateUpdate : public HostFunction<StateUpdate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_state_t State,
                        const_uint8_t_ptr InputPtr, __wasi_size_t InputSize);
};

class StateSign : public HostFunction<StateSign> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_state_t State,
                        __wasi_array_output_t /* Out */ ArrayOutputPtr);
};

class StateClose : public HostFunction<StateClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_state_t State);
};

class VerificationStateOpen : public HostFunction<VerificationStateOpen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_publickey_t Pk,
                        uint32_t /* Out */ SignatureVerificationStatePtr);
};

class VerificationStateUpdate : public HostFunction<VerificationStateUpdate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_verification_state_t State,
                        const_uint8_t_ptr InputPtr, __wasi_size_t InputSize);
};

class VerificationStateVerify : public HostFunction<VerificationStateVerify> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_verification_state_t State,
                        __wasi_signature_t Signature);
};

class VerificationStateClose : public HostFunction<VerificationStateClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_verification_state_t State);
};

class Close : public HostFunction<Close> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_signature_t Signature);
};

} // namespace Signatures
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge