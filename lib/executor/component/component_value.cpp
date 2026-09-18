// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "executor/component/executor.h"

#include "common/errinfo.h"
#include "common/spdlog.h"

namespace WasmEdge {
namespace Executor {

// Instantiate value section. See "include/executor/component/executor.h".
Expect<void>
ComponentExecutor::instantiate(Runtime::Instance::ComponentInstance &CompInst,
                               const AST::Component::ValueSection &ValSec) {
  for (const auto &Val : ValSec.getContent()) {
    // The validator decoded the payload against its type.
    const auto &Decoded = Val.getDecoded();
    if (!Decoded.has_value()) {
      spdlog::error(ErrCode::Value::NotValidated);
      spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Comp_Sec_Value));
      return Unexpect(ErrCode::Value::NotValidated);
    }
    CompInst.addValue(*Decoded);
  }
  return {};
}

} // namespace Executor
} // namespace WasmEdge
