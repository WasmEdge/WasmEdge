#pragma once

#include "stackmgr.h"
#include "storemgr.h"

namespace SSVM {
namespace RuntimeData {
  SSVM::StoreMgr Store;
  SSVM::StackMgr Stack;

} // namespace RuntimeData
} // namespace SSVM
