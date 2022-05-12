#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#if __wasi__
__attribute__((export_name("fib")))
#else
__attribute__((visibility("default")))
#endif
auto fib(int32_t n) -> int32_t;

#ifdef __cplusplus
}
#endif
