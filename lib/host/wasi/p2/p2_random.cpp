// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/component/random.h"
#include "host/wasi/p2/random.h"

#include <algorithm>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

Expect<std::vector<uint8_t>>
GetRandomBytes::body(Runtime::Component::CallingFrame &, uint64_t Len) {
  std::vector<uint8_t> Out(
      static_cast<size_t>(std::min(Len, WasiComponent::MaxRandomBytesPerCall)));
  WasiComponent::fillSecure(Out);
  return Out;
}

Expect<uint64_t> GetRandomU64::body(Runtime::Component::CallingFrame &) {
  return WasiComponent::secureU64();
}

Expect<std::vector<uint8_t>>
GetInsecureRandomBytes::body(Runtime::Component::CallingFrame &, uint64_t Len) {
  std::vector<uint8_t> Out(
      static_cast<size_t>(std::min(Len, WasiComponent::MaxRandomBytesPerCall)));
  WasiComponent::fillInsecure(Engine, Out);
  return Out;
}

Expect<uint64_t>
GetInsecureRandomU64::body(Runtime::Component::CallingFrame &) {
  return Engine();
}

Expect<std::tuple<uint64_t, uint64_t>>
InsecureSeed::body(Runtime::Component::CallingFrame &) {
  return std::make_tuple(WasiComponent::secureU64(),
                         WasiComponent::secureU64());
}

RandomInstance::RandomInstance()
    : ComponentInstance("wasi:random/random@0.2.12") {
  addHostFunc("get-random-bytes", std::make_unique<GetRandomBytes>());
  addHostFunc("get-random-u64", std::make_unique<GetRandomU64>());
}

InsecureInstance::InsecureInstance()
    : ComponentInstance("wasi:random/insecure@0.2.12") {
  addHostFunc("get-insecure-random-bytes",
              std::make_unique<GetInsecureRandomBytes>(Engine));
  addHostFunc("get-insecure-random-u64",
              std::make_unique<GetInsecureRandomU64>(Engine));
}

InsecureSeedInstance::InsecureSeedInstance()
    : ComponentInstance("wasi:random/insecure-seed@0.2.12") {
  addHostFunc("insecure-seed", std::make_unique<InsecureSeed>());
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeRandomInstances() {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  Out.push_back(std::make_unique<RandomInstance>());
  Out.push_back(std::make_unique<InsecureInstance>());
  Out.push_back(std::make_unique<InsecureSeedInstance>());
  return Out;
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
