// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

__attribute__((noinline)) int mix(int X) { return X * 31 + 7; }

__attribute__((export_name("scoped"))) int scoped(int N) {
  int Sum = mix(N);
  for (int I = 0; I < N; ++I) {
    int Step = mix(I);
    Sum += Step ^ I; // BREAK_SCOPED
  }
  return Sum;
}
