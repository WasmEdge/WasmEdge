//===-- ssvm/test/loader/moduleTest.cpp - AST module unit tests -----------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of AST module node and the main function.
///
//===----------------------------------------------------------------------===//

#include "loader/module.h"
#include <gtest/gtest.h>

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}