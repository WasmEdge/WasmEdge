// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/mock/log.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasiThreadsMock {

using namespace std::literals;

class ThreadSpawn : public Runtime::HostFunction<ThreadSpawn> {
public:
  Expect<void> body(const Runtime::CallingFrame &, int32_t) {
    printPluginMock("Wasi-Threads"sv);
    return Unexpect(ErrCode::Value::HostFuncError);
  }
};

} // namespace WasiThreadsMock
} // namespace Host
} // namespace WasmEdge
