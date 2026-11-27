// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors
#pragma once

#include "executor/coredump.h"
#include "runtime/instance/module.h"

#include <utility>
#include <vector>

namespace WasmEdge {
namespace Coredump {
namespace Detail {

struct MemoryPlan {
  Runtime::Instance::MemoryInstance *Instance = nullptr;
  uint64_t Pages = 0;
  std::vector<std::pair<uint64_t, std::vector<Byte>>> Segments;
};

struct TablePlan {
  Runtime::Instance::TableInstance *Instance = nullptr;
  std::vector<RefVariant> Refs;
};

struct RestorePlan {
  std::vector<MemoryPlan> Memories;
  std::vector<TablePlan> Tables;
  std::vector<std::pair<Runtime::Instance::GlobalInstance *, ValVariant>>
      Globals;
  std::vector<Runtime::Instance::DataInstance *> DataDrops;
  std::vector<Runtime::Instance::ElementInstance *> ElemDrops;
};

Expect<RestorePlan> preflight(const RestoreRequest &Req) noexcept;

} // namespace Detail
} // namespace Coredump
} // namespace WasmEdge
