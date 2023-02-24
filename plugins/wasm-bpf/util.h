#ifndef _UTIL
#define _UTIL
#include "runtime/instance/memory.h"
#include <cinttypes>
WasmEdge::Expect<const char*> read_c_str(
    WasmEdge::Runtime::Instance::MemoryInstance* memory, uint32_t ptr);

#endif