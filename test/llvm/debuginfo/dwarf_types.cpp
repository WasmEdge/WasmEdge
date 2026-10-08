// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

struct Point {
  int X;
  int Y;
};

struct Pair {
  Point M0;
};

char Tag;

__attribute__((export_name("run"))) int run(int V) {
  Pair P{};
  P.M0.X = V;
  return P.M0.X + Tag;
}
