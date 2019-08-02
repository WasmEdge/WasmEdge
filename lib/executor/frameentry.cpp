#include "executor/frameentry.h"

/// Constructor of frame entry. See "include/executor/frameentry.h".
FrameEntry::FrameEntry(
    unsigned int ModuleAddr, unsigned int Arity,
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    std::vector<std::pair<unsigned int, AST::ValType>> &LocalDef) {
  /// Set arity.
  FuncArity = Arity;

  /// Set parameters with arguments.
  for (auto It = Args.begin(); It != Args.end(); It++) {
    AST::ValType ArgType;
    (*It)->getType(ArgType);
    switch (ArgType) {
    case AST::ValType::I32: {
      int32_t Val = 0;
      (*It)->getValueI32(Val);
      Locals.push_back(Val);
      break;
    }
    case AST::ValType::I64: {
      int64_t Val = 0;
      (*It)->getValueI64(Val);
      Locals.push_back(Val);
      break;
    }
    case AST::ValType::F32: {
      float Val = 0.0;
      (*It)->getValueF32(Val);
      Locals.push_back(Val);
      break;
    }
    case AST::ValType::F64: {
      double Val = 0.0;
      (*It)->getValueF64(Val);
      Locals.push_back(Val);
      break;
    }
    default:
      break;
    }
  }

  /// Set local variables with initialization.
  for (auto It = LocalDef.begin(); It != LocalDef.end(); It++) {
    switch (It->second) {
    case AST::ValType::I32: {
      int32_t Val = 0;
      for (unsigned int i = 0; i < It->first; i++)
        Locals.push_back(Val);
      break;
    }
    case AST::ValType::I64: {
      int64_t Val = 0;
      for (unsigned int i = 0; i < It->first; i++)
        Locals.push_back(Val);
      break;
    }
    case AST::ValType::F32: {
      float Val = 0;
      for (unsigned int i = 0; i < It->first; i++)
        Locals.push_back(Val);
      break;
    }
    case AST::ValType::F64: {
      double Val = 0;
      for (unsigned int i = 0; i < It->first; i++)
        Locals.push_back(Val);
      break;
    }
    default:
      break;
    }
  }
}

/// Getter of module address. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::getModuleAddr(unsigned int &Addr) {
  Addr = ModAddr;
  return Executor::ErrCode::Success;
}

/// Getter of arity. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::getArity(unsigned int &Arity) {
  Arity = FuncArity;
  return Executor::ErrCode::Success;
}

/// Getter of I32 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::getValueI32(unsigned int Idx, int32_t &Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 0)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Val = std::get<0>(Locals[Idx]);
  return Executor::ErrCode::Success;
}

/// Getter of I64 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::getValueI64(unsigned int Idx, int64_t &Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 1)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Val = std::get<1>(Locals[Idx]);
  return Executor::ErrCode::Success;
}

/// Getter of F32 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::getValueF32(unsigned int Idx, float &Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 2)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Val = std::get<2>(Locals[Idx]);
  return Executor::ErrCode::Success;
}

/// Getter of F64 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::getValueF64(unsigned int Idx, double &Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 3)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Val = std::get<3>(Locals[Idx]);
  return Executor::ErrCode::Success;
}

/// Setter of I32 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::setValueI32(unsigned int Idx, int32_t Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 0)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Locals[Idx] = Val;
  return Executor::ErrCode::Success;
}

/// Setter of I64 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::setValueI64(unsigned int Idx, int64_t Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 1)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Locals[Idx] = Val;
  return Executor::ErrCode::Success;
}

/// Setter of F32 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::setValueF32(unsigned int Idx, float Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 2)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Locals[Idx] = Val;
  return Executor::ErrCode::Success;
}

/// Setter of F64 local by index. See "include/executor/frameentry.h".
Executor::ErrCode FrameEntry::setValueF64(unsigned int Idx, double Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::StackWrongEntry;
  /// Check if the type valid.
  if (Locals[Idx].index() != 3)
    return Executor::ErrCode::TypeNotMatch;
  /// Get value.
  Locals[Idx] = Val;
  return Executor::ErrCode::Success;
}