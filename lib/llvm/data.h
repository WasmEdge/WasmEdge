// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC
#pragma once

#include "llvm.h"
#include "llvm/data.h"

struct WasmEdge::LLVM::Data::DataContext {
  LLVM::Context LLContext;
  LLVM::Module LLModule;
  LLVM::TargetMachine TM;
  DataContext() noexcept : LLContext(), LLModule(LLContext, "wasm") {}
};
