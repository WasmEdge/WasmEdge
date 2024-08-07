// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/plugins/wasi_crypto/signatures/func.h - Signatures func --===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the signatures host functions of wasi-crypto.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "utils/hostfunction.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Signatures {

class Export : public HostFunction<Export> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t SigHandle,
                        uint32_t Encoding,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class Import : public HostFunction<Import> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t AlgPtr,
                        uint32_t AlgLen, uint32_t EncodedPtr,
                        uint32_t EncodedLen, uint32_t Encoding,
                        uint32_t /* Out */ SigHandlePtr);
};

class StateOpen : public HostFunction<StateOpen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t KpHandle,
                        uint32_t /* Out */ SigStatePtr);
};

class StateUpdate : public HostFunction<StateUpdate> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SigStateHandle, uint32_t InputPtr,
                        uint32_t InputSize);
};

class StateSign : public HostFunction<StateSign> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SigStateHandle,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class StateClose : public HostFunction<StateClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SigStateHandle);
};

class VerificationStateOpen : public HostFunction<VerificationStateOpen> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t SigPkHandle,
                        uint32_t /* Out */ VerificationStateHandlePtr);
};

class VerificationStateUpdate : public HostFunction<VerificationStateUpdate> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t SigStateHandle, uint32_t InputPtr,
                        uint32_t InputSize);
};

class VerificationStateVerify : public HostFunction<VerificationStateVerify> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t VerificationStateHandle, int32_t SigHandle);
};

class VerificationStateClose : public HostFunction<VerificationStateClose> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        int32_t VerificationStateHandle);
};

class Close : public HostFunction<Close> {
public:
  using HostFunction::HostFunction;
  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t SigHandle);
};

} // namespace Signatures
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
