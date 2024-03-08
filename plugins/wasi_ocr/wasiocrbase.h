// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "wasiocrenv.h"

namespace WasmEdge {
namespace Host {

template <typename T> class WasiOCR : public Runtime::HostFunction<T> {
public:
  WasiOCR(WASIOCR::WasiOCREnvironment &HostEnv)
      : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  WASIOCR::WasiOCREnvironment &Env;
};

} // namespace Host
} // namespace WasmEdge
