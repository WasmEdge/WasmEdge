#pragma once

#include "common.h"

class StoreMgr {};
class StackMgr {};
class RTDataMgr {
public:
  /// Executor::ErrCode setModule(std::unique_ptr<AST::Module> &Mod);

private:
  StoreMgr Store;
  StackMgr Stack;
};