// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {
namespace WasmEdgeStableDiffusionMock {

using namespace std::literals;
static inline constexpr const uint32_t kAbiVersionNotInstalled = 0U;
static inline constexpr const uint32_t kStableDiffusionError = 1U;

class AbiVersion : public Runtime::HostFunction<AbiVersion> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kAbiVersionNotInstalled;
  }
};

class SessionOpen : public Runtime::HostFunction<SessionOpen> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};

class SessionClose : public Runtime::HostFunction<SessionClose> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};

class Generate : public Runtime::HostFunction<Generate> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};

class ResultInfo : public Runtime::HostFunction<ResultInfo> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};

class ResultRead : public Runtime::HostFunction<ResultRead> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Stable-Diffusion"sv);
    return kStableDiffusionError;
  }
};

} // namespace WasmEdgeStableDiffusionMock
} // namespace Host
} // namespace WasmEdge
