// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/random.h - wasi:random host -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of `wasi:random@0.2.12`:
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
namespace WasiP2 {

/// `random.get-random-bytes: func(len: u64) -> list<u8>`
class GetRandomBytes : public Runtime::Component::HostFunction<GetRandomBytes> {
public:
  static constexpr const char *ParamNames[] = {"len"};
  Expect<std::vector<uint8_t>> body(Runtime::Component::CallingFrame &,
                                    uint64_t Len);
};

/// `random.get-random-u64: func() -> u64`
class GetRandomU64 : public Runtime::Component::HostFunction<GetRandomU64> {
public:
  Expect<uint64_t> body(Runtime::Component::CallingFrame &);
};

/// `insecure.get-insecure-random-bytes: func(len: u64) -> list<u8>`
class GetInsecureRandomBytes
    : public Runtime::Component::HostFunction<GetInsecureRandomBytes> {
public:
  static constexpr const char *ParamNames[] = {"len"};
  GetInsecureRandomBytes(Hash::RandomEngine &E) noexcept : Engine(E) {}
  Expect<std::vector<uint8_t>> body(Runtime::Component::CallingFrame &,
                                    uint64_t Len);

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

/// `insecure-seed.insecure-seed: func() -> tuple<u64, u64>`
class InsecureSeed : public Runtime::Component::HostFunction<InsecureSeed> {
public:
  Expect<std::tuple<uint64_t, uint64_t>>
  body(Runtime::Component::CallingFrame &);
};

/// `wasi:random/random@0.2.12`
class RandomInstance : public Runtime::Instance::ComponentInstance {
public:
  RandomInstance();
};

/// `wasi:random/insecure@0.2.12`, over its own fast generator.
class InsecureInstance : public Runtime::Instance::ComponentInstance {
public:
  InsecureInstance();

private:
  Hash::RandomEngine Engine;
};

/// `wasi:random/insecure-seed@0.2.12`
class InsecureSeedInstance : public Runtime::Instance::ComponentInstance {
public:
  InsecureSeedInstance();
};

/// The three instances of `wasi:random@0.2.12`.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeRandomInstances();

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
