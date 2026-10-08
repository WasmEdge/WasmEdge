// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

__attribute__((export_name("add3"))) int add3(int A, int B, int C) {
  int Sum = A + B;
  return Sum + C;
}
