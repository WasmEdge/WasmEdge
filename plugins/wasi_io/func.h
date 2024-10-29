// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "base.h"
#include "runtime/callingframe.h"
#include "type.h"

namespace WasmEdge {
namespace Host {

class DropOutputStream : public WasiIO<DropOutputStream> {
public:
  DropOutputStream(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisOutputStream);
};

class DropInputStream : public WasiIO<DropInputStream> {
public:
  DropInputStream(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisInputStream);
};

class DropStreamError : public WasiIO<DropStreamError> {
public:
  DropStreamError(WasiIOEnvironment &HostEnv) : WasiIO(HostEnv) {}
  Expect<void> body(uint32_t ThisError);
};

} // namespace Host
} // namespace WasmEdge
