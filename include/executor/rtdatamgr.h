#pragma once

#include "stackmgr.h"
#include "storemgr.h"

class RTDataMgr {
public:
private:
  SSVM::StoreMgr Store;
  SSVM::StackMgr Stack;
  /// TODO: remove this class.
};