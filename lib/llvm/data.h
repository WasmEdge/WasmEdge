// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC
#pragma once

#include "llvm.h"
#include "llvm/data.h"

struct WasmEdge::LLVM::Data::DataContext {
#if LLVM_VERSION_MAJOR >= 21
  LLVM::Context LLContext = LLVM::Context::create();
  LLVM::OrcThreadSafeContext TSContext = LLVM::OrcThreadSafeContext(LLContext);
  LLVM::Context getLLContext() noexcept { return LLContext; }
  LLVM::OrcThreadSafeContext &getTSContext() noexcept { return TSContext; }
#else
  LLVM::OrcThreadSafeContext TSContext;
  LLVM::Context getLLContext() noexcept { return TSContext.getContext(); }
  LLVM::OrcThreadSafeContext &getTSContext() noexcept { return TSContext; }
#endif
  LLVM::Module LLModule;
  LLVM::TargetMachine TM;
  std::string Prefix;
  DataContext() noexcept : LLModule(getLLContext(), "wasm") {}
  void resetModule() noexcept {
    LLModule = LLVM::Module(getLLContext(), "wasm");
  }
};
