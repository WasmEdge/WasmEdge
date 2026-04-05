// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "llvm/data.h"
#include "data.h"
#include "llvm.h"

namespace LLVM = WasmEdge::LLVM;

LLVM::Data::Data() noexcept : Context(std::make_unique<DataContext>()) {}

LLVM::Data::~Data() noexcept {}

LLVM::Data::Data(LLVM::Data &&RHS) noexcept : Context(std::move(RHS.Context)) {}
LLVM::Data &LLVM::Data::operator=(LLVM::Data &&RHS) noexcept {
  using std::swap;
  swap(Context, RHS.Context);
  return *this;
}
bool LLVM::Data::hasModule() const noexcept {
  return static_cast<bool>(Context->LLModule);
}
void LLVM::Data::resetModule() noexcept { Context->resetModule(); }
void LLVM::Data::setPrefix(std::string_view P) noexcept {
  Context->Prefix = std::string(P);
}
std::string_view LLVM::Data::getPrefix() const noexcept {
  return Context->Prefix;
}
