// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "util.h"

namespace WasmEdge {
namespace Host {

Expect<const char *> read_c_str(Runtime::Instance::MemoryInstance *memory,
                                uint32_t ptr) {
  uint32_t tail = ptr;
  while (true) {
    auto ch = memory->getBytes(tail, 1);
    if (!ch.has_value())
      return Unexpect(ch.error());
    if (ch.value()[0] == '\0')
      break;
    tail++;
  }
  uint32_t len = tail - ptr + 1;
  return memory->getPointer<const char *>(ptr, len);
}

} // namespace Host
} // namespace WasmEdge
