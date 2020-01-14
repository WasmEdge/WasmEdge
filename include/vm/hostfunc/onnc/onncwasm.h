// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "executor/hostfunc.h"

namespace SSVM {
namespace Executor {

template <typename T> class ONNCWasm : public HostFunction<T> {
public:
  ONNCWasm(const std::string &FuncName = "")
      : HostFunction<T>("onnc_wasm", FuncName, 0) {}
};

} // namespace Executor
} // namespace SSVM
