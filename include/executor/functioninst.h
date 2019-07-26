//===-- ssvm/executor/functioninst.h - Function Instance definition -------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the function instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/segment.h"
#include "common.h"
#include <memory>

class FunctionInstance {
public:
  Executor::ErrCode setModuleIdx(unsigned int Id);
  Executor::ErrCode setTypeIdx(unsigned int Id);
  Executor::ErrCode setCodeSegment(std::unique_ptr<AST::CodeSegment> CodeSeg);

private:
  unsigned int TypeIdx;
  unsigned int ModuleIdx;
  std::unique_ptr<AST::CodeSegment> Code;
};