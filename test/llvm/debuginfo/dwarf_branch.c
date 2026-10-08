// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

__attribute__((noinline)) void observe(int X) { __asm__ volatile("" ::"r"(X)); }

__attribute__((noinline)) int choose(int C, int A, int B) {
  int R;
  if (C) {
    observe(A); // THEN_PATH
    R = A * 3;
  } else {
    R = B + 7;
    observe(B); // ELSE_PATH
  }
  observe(R);
  return R;
}

__attribute__((export_name("run"))) int run(int C) { return choose(C, 4, 5); }
