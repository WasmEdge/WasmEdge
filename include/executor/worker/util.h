#pragma once

#include "ast/instruction.h"
#include "executor/entry/value.h"
#include "support/casting.h"

namespace SSVM {
namespace Executor {

namespace {

using OpCode = AST::Instruction::OpCode;

} // namespace

/// Retrieve value and casting to signed.
template <typename T>
inline typename std::enable_if_t<Support::IsWasmSignV<T>, T>
retrieveValue(const ValueEntry &Val) {
  std::make_unsigned_t<T> Value;
  Val.getValue(Value);
  return Support::toSigned(Value);
}

/// Retrieve value with original type.
template <typename T>
inline typename std::enable_if_t<Support::IsWasmBuiltInV<T>, T>
retrieveValue(const ValueEntry &Val) {
  T Value;
  Val.getValue(Value);
  return Value;
}

} // namespace Executor
} // namespace SSVM
