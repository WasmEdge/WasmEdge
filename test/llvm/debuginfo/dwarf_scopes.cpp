// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

namespace a {
int run(int X) { return X + 1; }
} // namespace a

namespace b {
int run(int X) { return X + 2; }
} // namespace b

struct Counter {
  int Value;
  int add(int X);
};

int Counter::add(int X) {
  Value += X;
  return Value;
}

enum class Color : unsigned char { Red, Green };

__attribute__((export_name("scopes"))) int scopes(int X) {
  Counter C{X};
  Color K = X ? Color::Green : Color::Red;
  return a::run(X) + b::run(X) + C.add(static_cast<int>(K));
}
