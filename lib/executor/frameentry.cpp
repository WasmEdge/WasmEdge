#include "executor/frameentry.h"

/// Constructor of frame entry. See "include/executor/frameentry.h".
FrameEntry::FrameEntry(
    unsigned int ModuleAddr, unsigned int Arity,
    std::vector<std::unique_ptr<ValueEntry>> &Args,
    std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs) {
  /// Set arity.
  this->Arity = Arity;

  /// Set parameters with arguments.
  for (auto Arg = Args.begin(); Arg != Args.end(); Arg++) {
    AST::ValType ArgType;
    AST::ValVariant ArgVal;
    (*Arg)->getType(ArgType);
    switch (ArgType) {
    case AST::ValType::I32: {
      int32_t Val = 0;
      (*Arg)->getValue(Val);
      ArgVal = Val;
      break;
    }
    case AST::ValType::I64: {
      int64_t Val = 0LL;
      (*Arg)->getValue(Val);
      ArgVal = Val;
      break;
    }
    case AST::ValType::F32: {
      float Val = 0.0;
      (*Arg)->getValue(Val);
      ArgVal = Val;
      break;
    }
    case AST::ValType::F64: {
      double Val = 0.0;
      (*Arg)->getValue(Val);
      ArgVal = Val;
      break;
    }
    default:
      break;
    }
    Locals.emplace_back(
        std::pair<AST::ValType, AST::ValVariant>(ArgType, ArgVal));
  }

  /// Set local variables with initialization.
  for (auto LocalDef = LocalDefs.begin(); LocalDef != LocalDefs.end();
       LocalDef++) {
    switch (LocalDef->second) {
    case AST::ValType::I32: {
      int32_t Val = 0;
      for (unsigned int i = 0; i < LocalDef->first; i++)
        Locals.emplace_back(
            std::pair<AST::ValType, AST::ValVariant>(AST::ValType::I32, Val));
      break;
    }
    case AST::ValType::I64: {
      int64_t Val = 0;
      for (unsigned int i = 0; i < LocalDef->first; i++)
        Locals.emplace_back(
            std::pair<AST::ValType, AST::ValVariant>(AST::ValType::I64, Val));
      break;
    }
    case AST::ValType::F32: {
      float Val = 0.0;
      for (unsigned int i = 0; i < LocalDef->first; i++)
        Locals.emplace_back(
            std::pair<AST::ValType, AST::ValVariant>(AST::ValType::F32, Val));
      break;
    }
    case AST::ValType::F64: {
      double Val = 0.0;
      for (unsigned int i = 0; i < LocalDef->first; i++)
        Locals.emplace_back(
            std::pair<AST::ValType, AST::ValVariant>(AST::ValType::F64, Val));
      break;
    }
    default:
      break;
    }
  }
}

/// Getter of local values by index. See "include/executor/frameentry.h".
template <typename T>
Executor::ErrCode FrameEntry::getValue(unsigned int Idx, T &Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::WrongLocalAddress;

  /// Get value.
  try {
    Val = std::get<T>(Locals[Idx].second);
  } catch (std::bad_variant_access E) {
    return Executor::ErrCode::TypeNotMatch;
  }
  return Executor::ErrCode::Success;
}

/// Setter of local values by index. See "include/executor/frameentry.h".
template <typename T>
Executor::ErrCode FrameEntry::setValue(unsigned int Idx, T Val) {
  /// Check if the index valid.
  if (Locals.size() <= Idx)
    return Executor::ErrCode::WrongLocalAddress;

  /// Check type.
  Executor::ErrCode Status = Executor::ErrCode::TypeNotMatch;
  switch (Locals[Idx].first) {
  case AST::ValType::I32:
    if (std::is_same<T, int32_t>::value)
      Status = Executor::ErrCode::Success;
    break;
  case AST::ValType::I64:
    if (std::is_same<T, int64_t>::value)
      Status = Executor::ErrCode::Success;
    break;
  case AST::ValType::F32:
    if (std::is_same<T, float>::value)
      Status = Executor::ErrCode::Success;
    break;
  case AST::ValType::F64:
    if (std::is_same<T, double>::value)
      Status = Executor::ErrCode::Success;
    break;
  default:
    break;
  }
  if (Status == Executor::ErrCode::Success)
    Locals[Idx].second = Val;
  return Status;
}