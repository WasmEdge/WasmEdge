// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2023 Second State INC

#pragma once

#include "ocr_env.h"

#include "common/errcode.h"
#include "runtime/hostfunc.h"

namespace WasmEdge {
namespace Host {
namespace WasmEdgeOCR {

template <typename T> class HostFunction : public Runtime::HostFunction<T> {
public:
  HostFunction(OCREnv &HostEnv) : Runtime::HostFunction<T>(0), Env(HostEnv) {}

protected:
  OCREnv &Env;
};

} // namespace WasmEdgeOCR
} // namespace Host
} // namespace WasmEdge
