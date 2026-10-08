// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

__attribute__((noinline)) int offset(int X) { return X + 1; }

__attribute__((noinline)) int scale(int X) {
  const int Fixed = 42;
  const short Negative = -2;
  return offset(X * Fixed) + Negative;
}

__attribute__((export_name("run"))) int run(int X) { return scale(X); }
