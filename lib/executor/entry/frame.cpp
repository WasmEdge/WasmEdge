#include "executor/entry/frame.h"

namespace SSVM {
namespace Executor {

/// Constructor of frame entry. See "include/executor/entry/frame.h".
FrameEntry::FrameEntry(
    unsigned int ModuleAddr, unsigned int Arity,
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs) {
  /// Set arity and module address.
  ModAddr = ModuleAddr;
  this->Arity = Arity;

  /// Set parameters with arguments.
  for (auto Arg = Args.rbegin(); Arg != Args.rend(); Arg++) {
    Locals.push_back(std::move(*Arg));
  }

  /// Set local variables with initialization.
  for (auto LocalDef = LocalDefs.begin(); LocalDef != LocalDefs.end();
       LocalDef++) {
    for (unsigned int i = 0; i < LocalDef->first; i++) {
      Locals.push_back(std::make_unique<ValueEntry>(LocalDef->second));
    }
  }
}

/// Getter of local values by index. See "include/executor/entry/frame.h".
ErrCode FrameEntry::getValue(unsigned int Idx, ValueEntry *&ValEntry) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return ErrCode::WrongLocalAddress;

  /// Get value.
  ValEntry = Locals[Idx].get();
  return ErrCode::Success;
}

/// Setter of local values by index. See "include/executor/entry/frame.h".
ErrCode FrameEntry::setValue(unsigned int Idx, const ValueEntry &ValEntry) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return ErrCode::WrongLocalAddress;

  /// Set value.
  if (ValEntry.getType() != Locals[Idx]->getType())
    return ErrCode::TypeNotMatch;
  Locals[Idx]->setValue(ValEntry);
  return ErrCode::Success;
}

} // namespace Executor
} // namespace SSVM
