// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

__attribute__((export_name("half"))) int half(int X) { return X / 2; }

__attribute__((export_name("quarter"))) int quarter(int X) {
  return half(half(X));
}
