// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

struct Outer {
  enum Kind { A, B } K;
  unsigned Low : 3;
  unsigned High : 5;
  int Tail;
};

__attribute__((export_name("nested"))) int nested(Outer::Kind K, Outer *O) {
  return K + O->Tail;
}

struct Base {
  int Value;
};
struct Plain : Base {
  int Extra;
};
struct Shared : virtual Base {
  int Extra;
};

__attribute__((export_name("bases"))) int bases(Plain *P) {
  Shared S;
  S.Value = P->Value;
  S.Extra = P->Extra;
  return S.Value + S.Extra;
}

struct __attribute__((packed)) Packed {
  char Head;
  unsigned Cross : 30;
  char Tail;
};

__attribute__((export_name("packed"))) unsigned packed(Packed *P) {
  return P->Cross;
}
