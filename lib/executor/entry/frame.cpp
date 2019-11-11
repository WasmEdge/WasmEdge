#include "executor/entry/frame.h"

namespace SSVM {
namespace Executor {

/// Initializer of frame entry. See "include/executor/entry/frame.h".
ErrCode FrameEntry::InitFrameEntry(
    unsigned int ModuleAddr, unsigned int FrameArity,
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs) {
  /// Set arity and module address.
  ModAddr = ModuleAddr;
  Arity = FrameArity;
  Locals.clear();

  /// Set parameters with arguments.
  for (auto Arg = Args.rbegin(); Arg != Args.rend(); Arg++) {
    /// Call ctor ValueEntry(const ValueEntry &VE)
    Locals.emplace_back(*Arg->get());
  }

  /// Set local variables with initialization.
  for (auto LocalDef = LocalDefs.begin(); LocalDef != LocalDefs.end();
       LocalDef++) {
    for (unsigned int i = 0; i < LocalDef->first; i++) {
      /// Call ctor ValueEntry(const AST::ValType &VT)
      Locals.emplace_back(LocalDef->second);
    }
  }
  return ErrCode::Success;
}

/// Initializer of frame entry. See "include/executor/entry/frame.h".
ErrCode FrameEntry::InitFrameEntry(unsigned int ModuleAddr,
                                   unsigned int FrameArity) {
  Arity = FrameArity;
  ModAddr = ModuleAddr;
  Locals.clear();
  return ErrCode::Success;
}

/// Getter of local values by index. See "include/executor/entry/frame.h".
ErrCode FrameEntry::getValue(unsigned int Idx, ValueEntry *&ValEntry) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return ErrCode::WrongLocalAddress;

  /// Get value.
  ValEntry = &Locals[Idx];
  return ErrCode::Success;
}

/// Setter of local values by index. See "include/executor/entry/frame.h".
ErrCode FrameEntry::setValue(unsigned int Idx, const ValueEntry &ValEntry) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return ErrCode::WrongLocalAddress;

  /// Set value.
  return Locals[Idx].setValue(ValEntry);
}

} // namespace Executor
} // namespace SSVM
