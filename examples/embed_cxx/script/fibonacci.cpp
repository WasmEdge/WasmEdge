#include "fibonacci.h"

auto fib(int32_t n) -> int32_t {
  if (n < 2) {
    return 1;
  }
  return fib(n - 2) + fib(n - 1);
}
