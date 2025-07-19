// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "host/wasi/vfs_io.h"
#include "common/defines.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string_view>

using namespace std::literals;

TEST(FStreamTest, IFStream) {
  WasmEdge::Host::WASI::Environ Env;
  Env.init({".:."s}, "test"s, {}, {});
  std::ofstream OutFile("test.txt");
  OutFile << "Hello, World!" << std::endl;
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&Env, "test.txt");
  EXPECT_TRUE(Stream.is_open());
  EXPECT_FALSE(Stream.fail());
  EXPECT_FALSE(Stream.eof());

  std::string Line;
  Stream.getline(Line);
  EXPECT_EQ(Line, "Hello, World!");

  Stream.close();
  EXPECT_FALSE(Stream.is_open());

  std::remove("test.txt");
}

TEST(FStreamTest, OFStream) {
  WasmEdge::Host::WASI::Environ Env;
  Env.init({".:."s}, "test"s, {}, {});
  WasmEdge::FStream::OFStream Stream(&Env, "output.txt");
  EXPECT_TRUE(Stream.is_open());
  EXPECT_FALSE(Stream.fail());

  Stream << "Writing to file.\n";
  Stream.close();
  EXPECT_FALSE(Stream.is_open());

  std::ifstream ReadStream("output.txt");
  std::string Line;
  getline(ReadStream, Line);
  EXPECT_EQ(Line, "Writing to file.");

  std::remove("output.txt");
}
