// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

typedef int Handler(int);

int Only = 41;

__attribute__((noinline)) static int twice(int X) { return X * 2; }

__attribute__((export_name("run"))) int run(int X) {
  int (*Plain)(int) = twice;
  Handler *Aliased = twice;
  return Plain(X) + Aliased(Only);
}
