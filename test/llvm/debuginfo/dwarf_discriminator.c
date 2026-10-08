// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

__attribute__((noinline)) int left(int);
__attribute__((noinline)) int right(int);
__attribute__((export_name("pick"))) int pick(int N) {
  int S = 0;
  // clang-format off
  for (int I = 0; I < N; ++I) { if (I & 1) S += left(I); else S += right(I); } // LOOP_LINE
  // clang-format on
  return S;
}
int left(int X) { return X * 3; }
int right(int X) { return X + 5; }
