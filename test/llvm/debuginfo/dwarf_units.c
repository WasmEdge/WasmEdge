// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

int helper(int X);

__attribute__((export_name("run"))) int run(int X) { return helper(X) + 1; }
