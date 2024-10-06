// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeImageMock {

using namespace std::literals;
static inline constexpr const uint32_t kWasmEdgeImageError = 1U;

class LoadJPG : public Runtime::HostFunction<LoadJPG> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Image"sv);
    return kWasmEdgeImageError;
  }
};

class LoadPNG : public Runtime::HostFunction<LoadPNG> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {
    printPluginMock("WasmEdge-Image"sv);
    return kWasmEdgeImageError;
  }
};

} // namespace WasmEdgeImageMock
} // namespace Host
} // namespace WasmEdge
