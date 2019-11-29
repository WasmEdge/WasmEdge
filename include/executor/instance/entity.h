// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/executor/instance/entity.h - Entity base definition ------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the base class of entity instances.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "executor/common.h"

#include <string>

namespace SSVM {
namespace Executor {
namespace Instance {

/// Base class of instance.
class Entity {
public:
  Entity() = default;
  virtual ~Entity() = default;

  /// Set the module name and entity name.
  ErrCode setNames(const std::string &Mod, const std::string &Entity);

  /// Match the module and function name.
  bool isName(const std::string &Mod, const std::string &Entity);

  /// Getter of module name.
  const std::string &getModName() const { return ModName; }

  /// Getter of entity name.
  const std::string &getEntityName() const { return EntityName; }

  /// Entity instance address in store manager.
  unsigned int Addr;

protected:
  std::string ModName;
  std::string EntityName;
};

} // namespace Instance
} // namespace Executor
} // namespace SSVM
