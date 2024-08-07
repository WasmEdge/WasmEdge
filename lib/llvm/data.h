// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "llvm.h"
#include "llvm/data.h"

struct WasmEdge::LLVM::Data::DataContext {
  LLVM::OrcThreadSafeContext TSContext;
  LLVM::Module LLModule;
  LLVM::TargetMachine TM;
  DataContext() noexcept : TSContext(), LLModule(LLContext(), "wasm") {}
  LLVM::Context LLContext() noexcept { return TSContext.getContext(); }
};
