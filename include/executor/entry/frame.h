//===-- ssvm/executor/entry/frame.h - Frame Entry class definition --------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Frame Entry class in stack manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/common.h"
#include "executor/common.h"
#include "value.h"
#include <memory>
#include <variant>
#include <vector>

namespace SSVM {
namespace Executor {

class Frame {
public:
  Frame() = default;
  ~Frame() = default;

  /// Initializer of frame entry.
  ///
  /// Initialize the frame with parameters and local variables.
  ///
  /// \param ModuleAddr the module instance address in store manager.
  /// \param FrameArity the return counts of this function type.
  /// \param Args the reversed arguments popped from stack for calling function.
  /// \param LocalDef the local variables definition in function instance.
  ///
  /// \returns ErrCode.
  Frame(unsigned int ModuleAddr, unsigned int FrameArity,
        std::vector<Value> &Args,
        const std::vector<std::pair<unsigned int, AST::ValType>> &LocalDefs);

  /// Initializer of frame entry.
  ///
  /// Initialize the frame with parameters and no local variable.
  ///
  /// \param ModuleAddr the module instance address in store manager.
  /// \param FrameArity the return counts of this function type.
  ///
  /// \returns ErrCode.
  Frame(unsigned int ModuleAddr, unsigned int FrameArity);

  /// Getter of module address.
  unsigned int getModuleAddr() const { return ModAddr; }

  /// Getter of arity.
  unsigned int getArity() const { return Arity; }

  /// Getter of local variables.
  ErrCode getValue(unsigned int Idx, Value *&ValEntry);

  /// Setter of local variables.
  ErrCode setValue(unsigned int Idx, const Value &ValEntry);

private:
  /// \name Data of frame entry.
  /// @{
  unsigned int ModAddr;
  unsigned int Arity;
  std::vector<Value> Locals;
  /// @}
};

} // namespace Executor
} // namespace SSVM
