// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC
#pragma once

#include <cstdint>
#include <optional>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;

class Task {
  // TODO:
  // opts: CanonicalOptions
  // inst: ComponentInstance
  // ft: FuncType
  // caller: Optional[Task]
  // on_return: Optional[Callable]
  // on_block: OnBlockCallback
  // events: list[EventCallback]
  // has_events: asyncio.Event
  // todo: int
};

class ResourceHandle {
public:
  ResourceHandle(uint32_t R, bool O)
      : Rep{R}, Own{O}, BorrowScope{std::nullopt} {}
  ResourceHandle(uint32_t R, bool O, std::optional<Task> B)
      : Rep{R}, Own{O}, BorrowScope{B} {}

  uint32_t getRep() noexcept { return Rep; }
  uint32_t getLendCount() noexcept { return LendCount; }

  bool isOwn() noexcept { return Own; }

private:
  uint32_t Rep;
  bool Own;
  std::optional<Task> BorrowScope;
  uint32_t LendCount = 0;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
