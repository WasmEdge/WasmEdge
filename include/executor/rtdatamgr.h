#pragma once

#include "stackmgr.h"
#include "storemgr.h"

class RTDataMgr {
public:
private:
  StoreMgr Store;
  StackMgr Stack;
};