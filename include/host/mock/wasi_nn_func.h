// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasiNNMock {

using namespace std::literals;
static inline constexpr const uint32_t kWASINNError = 1U;

class Load : public Runtime::HostFunction<Load> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-NN"sv);
    return kWASINNError;
  }
};

class InitExecCtx : public Runtime::HostFunction<InitExecCtx> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t) {
    printPluginMock("WASI-NN"sv);
    return kWASINNError;
  }
};

class SetInput : public Runtime::HostFunction<SetInput> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t) {
    printPluginMock("WASI-NN"sv);
    return kWASINNError;
  }
};

class GetOutput : public Runtime::HostFunction<GetOutput> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t, uint32_t,
                        uint32_t, uint32_t, uint32_t) {
    printPluginMock("WASI-NN"sv);
    return kWASINNError;
  }
};

class Compute : public Runtime::HostFunction<Compute> {
public:
  Expect<uint32_t> body(const Runtime::CallingFrame &, uint32_t) {
    printPluginMock("WASI-NN"sv);
    return kWASINNError;
  }
};

} // namespace WasiNNMock
} // namespace Host
} // namespace WasmEdge
