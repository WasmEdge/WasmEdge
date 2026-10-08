// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

struct Node {
  int Value;
  struct Node *Next;
};

static struct Node Nodes[3];
int Counter = 7;

static inline __attribute__((always_inline)) int twice(int X) {
  return X * 2;
}

__attribute__((noinline)) int sumList(struct Node *Head) {
  int Total = 0;
  for (struct Node *N = Head; N; N = N->Next) {
    Total += N->Value; // BREAK_SUMLIST
  }
  return Total;
}

__attribute__((noinline)) int fact(int N) {
  if (N <= 1) {
    return 1; // BREAK_FACT
  }
  return N * fact(N - 1);
}

__attribute__((export_name("run"))) int run(void) {
  for (int I = 0; I < 3; ++I) {
    Nodes[I].Value = I + 1;
    Nodes[I].Next = I < 2 ? &Nodes[I + 1] : 0;
  }
  Counter += twice(sumList(&Nodes[0])); // BREAK_RUN
  return Counter + fact(4);
}
