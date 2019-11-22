#include "executor/entry/frame.h"
#include <numeric>

namespace SSVM {
namespace Executor {

/// Initializer of frame entry. See "include/executor/entry/frame.h".
Frame::Frame(
    unsigned int ModuleAddr, unsigned int FrameArity, std::vector<Value> &Args,
    const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs)
    : ModAddr(ModuleAddr), Arity(FrameArity) {

  Locals.reserve(Args.size() +
                 std::accumulate(LocalDefs.cbegin(), LocalDefs.cend(),
                                 size_t(0), [](size_t value, const auto &Def) {
                                   return value + Def.first;
                                 }));

  /// Set parameters with arguments.
  std::copy(Args.crbegin(), Args.crend(), std::back_inserter(Locals));

  /// Set local variables with initialization.
  for (auto LocalDef = LocalDefs.begin(); LocalDef != LocalDefs.end();
       LocalDef++) {
    for (unsigned int i = 0; i < LocalDef->first; i++) {
      Locals.emplace_back(AST::ValueFromType(LocalDef->second));
    }
  }
}

/// Initializer of frame entry. See "include/executor/entry/frame.h".
Frame::Frame(unsigned int ModuleAddr, unsigned int FrameArity)
    : ModAddr(ModuleAddr), Arity(FrameArity) {}

/// Getter of local values by index. See "include/executor/entry/frame.h".
ErrCode Frame::getValue(unsigned int Idx, Value *&Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return ErrCode::WrongLocalAddress;

  /// Get value.
  Val = &Locals[Idx];
  return ErrCode::Success;
}

/// Setter of local values by index. See "include/executor/entry/frame.h".
ErrCode Frame::setValue(unsigned int Idx, const Value &Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return ErrCode::WrongLocalAddress;

  /// Set value.
  Locals[Idx] = Val;
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
