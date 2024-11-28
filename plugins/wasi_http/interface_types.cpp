// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "func.h"
#include "interface.h"

#include <memory>

namespace WasmEdge {
namespace Host {

WasiHttp_Types::WasiHttp_Types() : ComponentInstance("wasi:http/types@0.2.0") {
  addHostFunc("http-error-code", std::make_unique<Types::HttpErrorCode>(Env));
}

} // namespace Host
} // namespace WasmEdge