// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "type.h"

namespace WasmEdge {
namespace Host {

namespace StreamError {

T lastOperationFailed(uint32_t IOErr) noexcept { return T(IOErr); }
T closed() noexcept { return T{}; }

AST::Component::VariantTy ast() noexcept {
  return AST::Component::VariantTy{
      AST::Component::Case("last-operation-failed",
                           AST::Component::PrimValType::U32),
      AST::Component::Case("closed")};
}

} // namespace StreamError

} // namespace Host
} // namespace WasmEdge
