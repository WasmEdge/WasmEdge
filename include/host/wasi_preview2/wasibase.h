// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2023 Second State INC

#pragma once

#include "common/errcode.h"
#include "host/wasi_preview2/environ.h"
#include "runtime/callingframe.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiPreview2 : public Runtime::HostFunction<T> {
public:
  WasiPreview2(WASIPreview2::Environ &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WASIPreview2::Environ &Env;
};

} // namespace Host
} // namespace WasmEdge
