// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "host/wasi_nn/wasinncontext.h"
#include "runtime/importobj.h"

namespace WasmEdge {
namespace Host {

class WasiNNModule : public Runtime::ImportObject {
public:
  WasiNNModule();

private:
  WASINN::WasiNNContext Ctx;
};

} // namespace Host
} // namespace WasmEdge
