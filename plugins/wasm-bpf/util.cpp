// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "util.h"
using namespace WasmEdge;
Expect<const char*> read_c_str(Runtime::Instance::MemoryInstance* memory,
                               uint32_t ptr) {
    uint32_t tail = ptr;
    while (true) {
        auto val = memory->getBytes(tail, 1);
        if (!val.has_value())
            return Unexpect(val.error());
        if (val.value()[0] == '\0')
            break;
        tail++;
    }
    uint32_t len = tail - ptr + 1;
    return memory->getPointer<char*>(ptr, len);
}