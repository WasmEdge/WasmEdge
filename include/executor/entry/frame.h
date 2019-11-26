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
  /// \param StackSize current stack size.
  /// \param Coarity the return counts of this function type.
  ///
  /// \returns ErrCode.
  Frame(unsigned int ModuleAddr, unsigned int StackSize, unsigned int Coarity)
      : ModAddr(ModuleAddr), StackSize(StackSize), Coarity(Coarity) {}

  /// Getter of module address.
  unsigned int getModuleAddr() const { return ModAddr; }

  /// Getter of stack size.
  unsigned int getStackSize() const { return StackSize; }

  /// Getter of arity.
  unsigned int getCoarity() const { return Coarity; }

  /// Getter for stack offset of local values by index.
  unsigned int getOffset(unsigned int Idx) const { return StackSize + Idx; }

private:
  /// \name Data of frame entry.
  /// @{
  unsigned int ModAddr;
  unsigned int StackSize;
  unsigned int Coarity;
  /// @}
};

} // namespace Executor
} // namespace SSVM
