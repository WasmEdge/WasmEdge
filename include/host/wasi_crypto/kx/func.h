// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasi_crypto/kx/func.h - Key Exchange func -------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the key exchange func module of wasi-crypto
///
//===----------------------------------------------------------------------===//
#pragma once

#include "host/wasi_crypto/utils/hostfunction.h"

namespace WasmEdge {
namespace Host {
namespace WasiCrypto {
namespace Kx {

class Dh : public HostFunction<Dh> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t PkHandle, int32_t SkHandle,
                        uint32_t /* Out */ ArrayOutputHandlePtr);
};

class Encapsulate : public HostFunction<Encapsulate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t PkHandle, uint32_t /* Out */ SecretPtr,
                        uint32_t /* Out */ EncapsulatedSecretPtr);
};

class Decapsulate : public HostFunction<Decapsulate> {
public:
  using HostFunction::HostFunction;

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t SkHandle, uint32_t EncapsulatedSecretPtr,
                        uint32_t EncapsulatedSecretLen,
                        uint32_t /* Out */ SecretPtr);
};

} // namespace Kx
} // namespace WasiCrypto
} // namespace Host
} // namespace WasmEdge
