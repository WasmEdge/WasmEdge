// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/random.h - wasi:random host -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:random@0.3.1`:
/// `random`, `insecure`, and `insecure-seed`.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/hash.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/instance/component/component.h"

#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

/// `random.get-random-bytes: func(max-len: u64) -> list<u8>`
class GetRandomBytes : public Runtime::Component::HostFunction<GetRandomBytes> {
public:
  static constexpr const char *ParamNames[] = {"max-len"};
  Expect<std::vector<uint8_t>> body(Runtime::Component::CallingFrame &,
                                    uint64_t MaxLen);
};

/// `random.get-random-u64: func() -> u64`
class GetRandomU64 : public Runtime::Component::HostFunction<GetRandomU64> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `insecure.get-insecure-random-bytes: func(max-len: u64) -> list<u8>`
class GetInsecureRandomBytes
    : public Runtime::Component::HostFunction<GetInsecureRandomBytes> {
public:
  static constexpr const char *ParamNames[] = {"max-len"};
  GetInsecureRandomBytes(Hash::RandomEngine &E) noexcept : Engine(E) {}
  Expect<std::vector<uint8_t>> body(Runtime::Component::CallingFrame &,
                                    uint64_t MaxLen);

private:
  Hash::RandomEngine &Engine;
};

/// `insecure.get-insecure-random-u64: func() -> u64`
class GetInsecureRandomU64
    : public Runtime::Component::HostFunction<GetInsecureRandomU64> {
public:
  GetInsecureRandomU64(Hash::RandomEngine &E) noexcept : Engine(E) {}
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);

private:
  Hash::RandomEngine &Engine;
};

/// `insecure-seed.get-insecure-seed: func() -> tuple<u64, u64>`
class GetInsecureSeed
    : public Runtime::Component::HostFunction<GetInsecureSeed> {
public:
  Expect<std::tuple<uint64_t, uint64_t>>
  body(Runtime::Component::CallingFrame &);
};

/// `wasi:random/random@0.3.1`
class RandomInstance : public Runtime::Instance::ComponentInstance {
public:
  RandomInstance();
};

/// `wasi:random/insecure@0.3.1`, over its own fast generator.
class InsecureInstance : public Runtime::Instance::ComponentInstance {
public:
  InsecureInstance();

private:
  Hash::RandomEngine Engine;
};

/// `wasi:random/insecure-seed@0.3.1`
class InsecureSeedInstance : public Runtime::Instance::ComponentInstance {
public:
  InsecureSeedInstance();
};

/// The three instances of `wasi:random@0.3.1`.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeRandomInstances();

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
