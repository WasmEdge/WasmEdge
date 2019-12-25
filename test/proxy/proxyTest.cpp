// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/test/option/optionTest.cpp - Option parsing tests ------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents unit tests of parsing options for SSVMRPC.
///
//===----------------------------------------------------------------------===//

#include "proxy/cmdparser.h"
#include "gtest/gtest.h"

#include <cstdio>
#include <cstring>
#include <iostream>

namespace {

TEST(OptionTest, Parse__argument) {
  // SSVM --input_file=/home/application_uuid/service_id/timestamp/input.json
  // --output_file=/home/application_uuid/service_id/timestamp/output.json
  // --bytecode_file=/home/application_uuid/bytecode.wasm
  int argc = 4;
  char **argv = new char *[argc];
  argv[0] = (char *)malloc(sizeof(char) * 5);
  strcpy(argv[0], "SSVM");
  argv[1] = (char *)malloc(sizeof(char) * 69);
  strcpy(argv[1],
         "--input_file=/home/application_uuid/service_id/timestamp/input.json");
  argv[2] = (char *)malloc(sizeof(char) * 70);
  strcpy(
      argv[2],
      "--output_file=/home/application_uuid/service_id/timestamp/output.json");
  argv[3] = (char *)malloc(sizeof(char) * 54);
  strcpy(argv[3], "--bytecode_file=/home/application_uuid/bytecode.wasm");

  SSVM::Proxy::CmdParser Parser;
  Parser.parseCommandLine(argc, argv);
  EXPECT_EQ(Parser.getInputJSONPath(),
            "/home/application_uuid/service_id/timestamp/input.json");
  EXPECT_EQ(Parser.getOutputJSONPath(),
            "/home/application_uuid/service_id/timestamp/output.json");
  EXPECT_EQ(Parser.getWasmPath(), "/home/application_uuid/bytecode.wasm");

  free(argv[0]);
  free(argv[1]);
  free(argv[2]);
  free(argv[3]);
  free(argv);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
