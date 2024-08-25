// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/symmetric/func.h - Symmetric funcs ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the symmetric host functions of wasi-crypto.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "utils/hostfunction.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Symmetric {

class KeyGenerate : public HostFunction<KeyGenerate> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgPtr,
                        uint32_t AlgLen, uint32_t OptOptionsPtr,
                        uint32_t /* Out */ KeyHandlePtr);
};

class KeyImport : public HostFunction<KeyImport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgPtr,
                        uint32_t AlgLen, uint32_t RawPtr, uint32_t RawLen,
                        uint32_t /* Out */ KeyHandlePtr);
};

class KeyExport : public HostFunction<KeyExport> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KeyHandle,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class KeyClose : public HostFunction<KeyClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KeyHandle);
};

class KeyGenerateManaged : public HostFunction<KeyGenerateManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, uint32_t AlgPtr,
                        uint32_t AlgLen, uint32_t OptOptionsPtr,
                        uint32_t /* Out */ KeyHandlePtr);
};

class KeyStoreManaged : public HostFunction<KeyStoreManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, int32_t KeyHandle,
                        uint32_t KeyIdPtr, uint32_t KeyIdMaxLen);
};

class KeyReplaceManaged : public HostFunction<KeyReplaceManaged> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, int32_t OldKeyHandle,
                        int32_t NewKeyHandle, uint32_t /* Out */ KeyVersionPtr);
};

class KeyId : public HostFunction<KeyId> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KeyHandle,
                        uint32_t KeyIdPtr, uint32_t KeyIdMaxLen,
                        uint32_t /* Out */ SizePtr,
                        uint32_t /* Out */ KeyVersionPtr);
};

class KeyFromId : public HostFunction<KeyFromId> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SecretsManagerHandle, uint32_t KeyIdPtr,
                        uint32_t KeyIdLen, uint64_t KeyVersion,
                        uint32_t /* Out */ KeyHandlePtr);
};

class StateOpen : public HostFunction<StateOpen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgPtr,
                        uint32_t AlgLen, uint32_t OptKeyHandlePtr,
                        uint32_t OptOptionsPtr, uint32_t /* Out */ StatePtr);
};

class StateClone : public HostFunction<StateClone> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t /* Out */ StatePtr);
};

class StateOptionsGet : public HostFunction<StateOptionsGet> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t NamePtr, uint32_t NameLen, uint32_t ValuePtr,
                        uint32_t ValueLen, uint32_t /* Out */ SizePtr);
};

class StateOptionsGetU64 : public HostFunction<StateOptionsGetU64> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t NamePtr, uint32_t NameLen,
                        uint32_t /* Out */ U64Ptr);
};

class StateClose : public HostFunction<StateClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t StateHandle);
};

class StateAbsorb : public HostFunction<StateAbsorb> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t DataPtr, uint32_t DataLen);
};

class StateSqueeze : public HostFunction<StateSqueeze> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t OutPtr, uint32_t OutLen);
};

class StateSqueezeTag : public HostFunction<StateSqueezeTag> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t /* Out */ TagHandlePtr);
};

class StateSqueezeKey : public HostFunction<StateSqueezeKey> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t AlgPtr, uint32_t AlgLen,
                        uint32_t /* Out */ KeyHandlePtr);
};

class StateMaxTagLen : public HostFunction<StateMaxTagLen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t /* Out */ SizePtr);
};

class StateEncrypt : public HostFunction<StateEncrypt> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t OutPtr, uint32_t OutLen, uint32_t DataPtr,
                        uint32_t DataLen, uint32_t /* Out */ SizePtr);
};

class StateEncryptDetached : public HostFunction<StateEncryptDetached> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t OutPtr, uint32_t OutLen, uint32_t DataPtr,
                        uint32_t DataLen, uint32_t /* Out */ TagHandlePtr);
};

class StateDecrypt : public HostFunction<StateDecrypt> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t OutPtr, uint32_t OutLen, uint32_t DataPtr,
                        uint32_t DataLen, uint32_t /* Out */ SizePtr);
};

class StateDecryptDetached : public HostFunction<StateDecryptDetached> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t StateHandle,
                        uint32_t OutPtr, uint32_t OutLen, uint32_t DataPtr,
                        uint32_t DataLen, uint32_t RawTagPtr,
                        uint32_t RawTagLen, uint32_t /* Out */ SizePtr);
};

class StateRatchet : public HostFunction<StateRatchet> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t StateHandle);
};

class TagLen : public HostFunction<TagLen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t TagHandle,
                        uint32_t /* Out */ SizePtr);
};

class TagPull : public HostFunction<TagPull> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t TagHandle,
                        uint32_t BufPtr, uint32_t BufLen,
                        uint32_t /* Out */ SizePtr);
};

class TagVerify : public HostFunction<TagVerify> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t TagHandle,
                        uint32_t RawTagPtr, uint32_t RawTagLen);
};

class TagClose : public HostFunction<TagClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t TagHandle);
};

} // namespace Symmetric
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
