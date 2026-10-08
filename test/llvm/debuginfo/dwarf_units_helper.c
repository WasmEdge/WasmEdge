// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

static int Bias = 3;

int helper(int X) {
  int Scaled = X * Bias;
  return Scaled + 2;
}
