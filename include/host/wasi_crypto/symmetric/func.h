// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "host/wasi_crypto/ctx.h"
#include "runtime/hostfunc.h"
#include "runtime/instance/memory.h"
#include "wasi_crypto/api.hpp"

namespace WasmEdge {
namespace Host {
namespace WASICrypto {
namespace Symmetric {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(WasiCryptoContext &HostCtx)
      : Runtime::HostFunction<T>(0), Ctx(HostCtx) {}

protected:
  WasiCryptoContext &Ctx;
};

class KeyGenerate : public HostFunction<KeyGenerate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                        uint32_t OptionsPtr, uint32_t /* Out */ KeyPtr);
};

class KeyImport : public HostFunction<KeyImport> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                        const_uint8_t_ptr RawPtr, __wasi_size_t RawLen,
                        uint32_t /* Out */ KeyPtr);
};

class KeyExport : public HostFunction<KeyExport> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_key_t SymmetricKey,
                        uint32_t /* Out */ ArrayOutputPtr);
};

class KeyClose : public HostFunction<KeyClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_key_t SymmetricKey);
};

class KeyGenerateManaged : public HostFunction<KeyGenerateManaged> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                        uint32_t OptOptionsPtr, uint32_t /* Out */ KeyPtr);
};

class KeyStoreManaged : public HostFunction<KeyStoreManaged> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        __wasi_symmetric_key_t SymmetricKey,
                        uint8_t_ptr SymmetricKeyIdPtr,
                        __wasi_size_t SymmetricKeyIdMaxLen);
};

class KeyReplaceManaged : public HostFunction<KeyReplaceManaged> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        __wasi_symmetric_key_t SymmetricKeyOld,
                        __wasi_symmetric_key_t SymmetricKeyNew,
                        uint32_t /* Out */ VersionPtr);
};

class KeyId : public HostFunction<KeyId> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_key_t SymmetricKey,
                        uint8_t_ptr SymmetricKeyId,
                        __wasi_size_t SymmetricKeyIdMaxLen,
                        uint32_t /* Out */ SizePtr,
                        uint32_t /* Out */ VersionPtr);
};

class KeyFromId : public HostFunction<KeyFromId> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_secrets_manager_t SecretsManager,
                        uint8_t_ptr SymmetricKeyIdPtr,
                        __wasi_size_t SymmetricKeyIdLen,
                        __wasi_version_t SymmetricKeyVersion,
                        uint32_t /* Out */ KeyPtr);
};

class StateOpen : public HostFunction<StateOpen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                        uint32_t OptKeyPtr, uint32_t OptOptionsPtr,
                        uint32_t /* Out */ StatePtr);
};

class StateOptionsGet : public HostFunction<StateOptionsGet> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr NamePtr, __wasi_size_t NameLen,
                        uint8_t_ptr ValuePtr, __wasi_size_t ValueLen,
                        uint32_t /* Out */ SizePtr);
};

class StateOptionsGetU64 : public HostFunction<StateOptionsGetU64> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr NamePtr, __wasi_size_t NameLen,
                        uint32_t /* Out */ U64Ptr);
};

class StateClose : public HostFunction<StateClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle);
};

class StateAbsorb : public HostFunction<StateAbsorb> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr DataPtr, __wasi_size_t DataLen);
};

class StateSqueeze : public HostFunction<StateSqueeze> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen);
};

class StateSqueezeTag : public HostFunction<StateSqueezeTag> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        uint32_t /* Out */ TagHandlePtr);
};

class StateSqueezeKey : public HostFunction<StateSqueezeKey> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        const_uint8_t_ptr AlgPtr, __wasi_size_t AlgLen,
                        uint32_t /* Out */ KeyPtr);
};

class StateMaxTagLen : public HostFunction<StateMaxTagLen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle,
                        uint32_t /* Out */ SizePtr);
};

class StateEncrypt : public HostFunction<StateEncrypt> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint32_t /* Out */ SizePtr);
};

class StateEncryptDetached : public HostFunction<StateEncryptDetached> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint32_t /* Out */ KeyPtr);
};

class StateDecrypt : public HostFunction<StateDecrypt> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint32_t /* Out */ SizePtr);
};

class StateDecryptDetached : public HostFunction<StateDecryptDetached> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle, uint8_t_ptr OutPtr,
                        __wasi_size_t OutLen, const_uint8_t_ptr DataPtr,
                        __wasi_size_t DataLen, uint8_t_ptr RawTagPtr,
                        __wasi_size_t RawTagLen, uint32_t /* Out */ SizePtr);
};

class StateRatchet : public HostFunction<StateRatchet> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_state_t Handle);
};

class TagLen : public HostFunction<TagLen> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag,
                        uint32_t /* Out */ SizePtr);
};

class TagPull : public HostFunction<TagPull> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag, uint8_t_ptr BufPtr,
                        __wasi_size_t BufLen, uint32_t /* Out */ SizePtr);
};

class TagVerify : public HostFunction<TagVerify> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag,
                        uint8_t_ptr RawTagPtr, __wasi_size_t RawTagLen);
};

class TagClose : public HostFunction<TagClose> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        __wasi_symmetric_tag_t SymmetricTag);
};

} // namespace Symmetric
} // namespace WASICrypto
} // namespace Host
} // namespace WasmEdge
