// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2025 Second State INC

#include "host/wasi/vfs_io.h"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <string_view>

using namespace std::literals;

class VfsIoTest : public ::testing::Test {
protected:
  void SetUp() override { TestEnv.init({".:."s}, "test"s, {}, {}); }

  void TearDown() override {
    // Clean up test files
    std::remove("test.txt");
    std::remove("output.txt");
    std::remove("empty.txt");
    std::remove("large.txt");
    std::remove("binary.dat");
    std::remove("seektest.txt");
    std::remove("numeric.txt");
  }

  WasmEdge::Host::WASI::Environ TestEnv;
};

// Basic IFStream Tests
TEST_F(VfsIoTest, IFStreamBasicRead) {
  std::ofstream OutFile("test.txt");
  OutFile << "Hello, World!" << std::endl;
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());
  EXPECT_FALSE(Stream.fail());
  EXPECT_FALSE(Stream.eof());
  EXPECT_TRUE(Stream.good());
  EXPECT_TRUE(static_cast<bool>(Stream));

  std::string Line;
  Stream.getline(Line);
  EXPECT_EQ(Line, "Hello, World!");

  Stream.close();
  EXPECT_FALSE(Stream.is_open());
}

TEST_F(VfsIoTest, IFStreamReadMethod) {
  std::ofstream OutFile("test.txt");
  OutFile << "Test data for reading";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());

  char Buffer[20];
  Stream.read(Buffer, 9);
  Buffer[9] = '\0';
  EXPECT_STREQ(Buffer, "Test data");

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamReadsomeMethod) {
  std::ofstream OutFile("test.txt");
  OutFile << "Sample text for readsome test";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());

  char Buffer[50];
  std::streamsize BytesRead = Stream.readsome(Buffer, 6);
  Buffer[BytesRead] = '\0';
  EXPECT_STREQ(Buffer, "Sample");
  EXPECT_GT(BytesRead, 0);

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamGetMethod) {
  std::ofstream OutFile("test.txt");
  OutFile << "ABC";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());

  int FirstChar = Stream.get();
  int SecondChar = Stream.get();
  int ThirdChar = Stream.get();

  EXPECT_EQ(FirstChar, 'A');
  EXPECT_EQ(SecondChar, 'B');
  EXPECT_EQ(ThirdChar, 'C');

  int EofChar = Stream.get();
  EXPECT_EQ(EofChar, EOF);
  EXPECT_TRUE(Stream.eof());

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamGetlineWithCustomDelimiter) {
  std::ofstream OutFile("test.txt");
  OutFile << "Line1;Line2;Line3";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());

  std::string Line1, Line2, Line3;
  Stream.getline(Line1, ';');
  Stream.getline(Line2, ';');
  Stream.getline(Line3, ';');

  EXPECT_EQ(Line1, "Line1");
  EXPECT_EQ(Line2, "Line2");
  EXPECT_EQ(Line3, "Line3");

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamGetlineReturnValue) {
  std::ofstream OutFile("test.txt");
  OutFile << "Single line";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());

  std::string Line = Stream.getline();
  EXPECT_EQ(Line, "Single line");

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamNumericOperators) {
  std::ofstream OutFile("numeric.txt");
  OutFile << "42 3.14 100 2.718";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "numeric.txt");
  EXPECT_TRUE(Stream.is_open());

  int IntValue;
  float FloatValue;
  long LongValue;
  double DoubleValue;

  Stream >> IntValue >> FloatValue >> LongValue >> DoubleValue;

  EXPECT_EQ(IntValue, 42);
  EXPECT_FLOAT_EQ(FloatValue, 3.14f);
  EXPECT_EQ(LongValue, 100L);
  EXPECT_DOUBLE_EQ(DoubleValue, 2.718);

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamStringOperator) {
  std::ofstream OutFile("test.txt");
  OutFile << "Hello World Test";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "test.txt");
  EXPECT_TRUE(Stream.is_open());

  std::string Word1, Word2, Word3;
  Stream >> Word1 >> Word2 >> Word3;

  EXPECT_EQ(Word1, "Hello");
  EXPECT_EQ(Word2, "World");
  EXPECT_EQ(Word3, "Test");

  Stream.close();
}

// Position and Seeking Tests
TEST_F(VfsIoTest, IFStreamPositionOperations) {
  std::ofstream OutFile("seektest.txt");
  OutFile << "0123456789";
  OutFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "seektest.txt");
  EXPECT_TRUE(Stream.is_open());

  std::streampos InitialPos = Stream.tellg();
  EXPECT_EQ(InitialPos, 0);

  char Buffer[6];
  Stream.read(Buffer, 5);
  std::streampos CurrentPos = Stream.tellg();
  EXPECT_EQ(CurrentPos, 5);

  Stream.seekg(2);
  CurrentPos = Stream.tellg();
  EXPECT_EQ(CurrentPos, 2);

  Stream.seekg(3, std::ios_base::cur);
  CurrentPos = Stream.tellg();
  EXPECT_EQ(CurrentPos, 5);

  Stream.seekg(-2, std::ios_base::end);
  CurrentPos = Stream.tellg();
  EXPECT_EQ(CurrentPos, 8);

  Stream.close();
}

// Basic OFStream Tests
TEST_F(VfsIoTest, OFStreamBasicWrite) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "output.txt");
  EXPECT_TRUE(Stream.is_open());
  EXPECT_FALSE(Stream.fail());
  EXPECT_TRUE(Stream.good());
  EXPECT_TRUE(static_cast<bool>(Stream));

  Stream << "Writing to file.\n";
  Stream.close();
  EXPECT_FALSE(Stream.is_open());

  std::ifstream ReadStream("output.txt");
  std::string Line;
  getline(ReadStream, Line);
  EXPECT_EQ(Line, "Writing to file.");
}

TEST_F(VfsIoTest, OFStreamWriteMethod) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "output.txt");
  EXPECT_TRUE(Stream.is_open());

  const char *Data = "Binary data test";
  Stream.write(Data, strlen(Data));
  Stream.close();

  std::ifstream ReadStream("output.txt");
  std::string Content;
  getline(ReadStream, Content);
  EXPECT_EQ(Content, "Binary data test");
}

TEST_F(VfsIoTest, OFStreamPutMethod) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "output.txt");
  EXPECT_TRUE(Stream.is_open());

  Stream.put('A').put('B').put('C');
  Stream.close();

  std::ifstream ReadStream("output.txt");
  std::string Content;
  getline(ReadStream, Content);
  EXPECT_EQ(Content, "ABC");
}

TEST_F(VfsIoTest, OFStreamFlushMethod) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "output.txt");
  EXPECT_TRUE(Stream.is_open());

  Stream << "Test data";
  Stream.flush();
  EXPECT_TRUE(Stream.good());

  Stream.close();
}

TEST_F(VfsIoTest, OFStreamNumericOperators) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "numeric.txt");
  EXPECT_TRUE(Stream.is_open());

  int IntValue = 42;
  float FloatValue = 3.14f;
  long LongValue = 100L;
  double DoubleValue = 2.718;

  Stream << IntValue << " " << FloatValue << " " << LongValue << " "
         << DoubleValue;
  Stream.close();

  std::ifstream ReadStream("numeric.txt");
  std::string Content;
  getline(ReadStream, Content);
  EXPECT_EQ(Content, "42 3.14 100 2.718");
}

// OFStream Position Operations
TEST_F(VfsIoTest, OFStreamPositionOperations) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "output.txt");
  EXPECT_TRUE(Stream.is_open());

  std::streampos InitialPos = Stream.tellp();
  EXPECT_EQ(InitialPos, 0);

  Stream << "Hello";
  std::streampos CurrentPos = Stream.tellp();
  EXPECT_EQ(CurrentPos, 5);

  Stream.seekp(0);
  Stream << "X";

  Stream.close();

  std::ifstream ReadStream("output.txt");
  std::string Content;
  getline(ReadStream, Content);
  EXPECT_EQ(Content, "Xello");
}

TEST_F(VfsIoTest, OFStreamSeekOperations) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "output.txt");
  EXPECT_TRUE(Stream.is_open());

  Stream << "0123456789";

  Stream.seekp(5);
  std::streampos Pos = Stream.tellp();
  EXPECT_EQ(Pos, 5);

  Stream.seekp(2, std::ios_base::cur);
  Pos = Stream.tellp();
  EXPECT_EQ(Pos, 7);

  Stream.seekp(3, std::ios_base::beg);
  Pos = Stream.tellp();
  EXPECT_EQ(Pos, 3);

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamNonExistentFile) {
  WasmEdge::FStream::IFStream Stream(&TestEnv, "non_existent_file.txt");
  EXPECT_FALSE(Stream.is_open());
  EXPECT_TRUE(Stream.fail());
  EXPECT_FALSE(Stream.good());
  EXPECT_FALSE(static_cast<bool>(Stream));
}

TEST_F(VfsIoTest, IFStreamEmptyFile) {
  std::ofstream EmptyFile("empty.txt");
  EmptyFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "empty.txt");
  EXPECT_TRUE(Stream.is_open());
  EXPECT_FALSE(Stream.eof());

  int FirstChar = Stream.get();
  EXPECT_EQ(FirstChar, EOF);
  EXPECT_TRUE(Stream.eof());

  Stream.close();
}

TEST_F(VfsIoTest, IFStreamLargeFile) {
  std::ofstream LargeFile("large.txt");
  for (int I = 0; I < 5000; ++I) {
    LargeFile << "Line " << I << std::endl;
  }
  LargeFile.close();

  WasmEdge::FStream::IFStream Stream(&TestEnv, "large.txt");
  EXPECT_TRUE(Stream.is_open());

  std::string FirstLine;
  Stream.getline(FirstLine);
  EXPECT_EQ(FirstLine, "Line 0");

  Stream.seekg(0, std::ios_base::end);
  EXPECT_TRUE(Stream.eof() || Stream.tellg() > 0);

  Stream.close();
}

TEST_F(VfsIoTest, OFStreamLargeWrite) {
  WasmEdge::FStream::OFStream Stream(&TestEnv, "large.txt");
  EXPECT_TRUE(Stream.is_open());

  Stream.setChunkSize(10);

  std::string LargeData(1000, 'A');
  Stream << LargeData;
  Stream.close();

  std::ifstream ReadStream("large.txt");
  std::string Content((std::istreambuf_iterator<char>(ReadStream)),
                      std::istreambuf_iterator<char>());
  EXPECT_EQ(Content.length(), 1000);
  EXPECT_EQ(Content, LargeData);
}

// Binary Data Tests
TEST_F(VfsIoTest, BinaryDataHandling) {
  WasmEdge::FStream::OFStream OutStream(&TestEnv, "binary.dat");
  EXPECT_TRUE(OutStream.is_open());

  const char BinaryData[] = {0x01, 0x02, 0x00, 0x03, 0x04};
  OutStream.write(BinaryData, sizeof(BinaryData));
  OutStream.close();

  WasmEdge::FStream::IFStream InStream(&TestEnv, "binary.dat");
  EXPECT_TRUE(InStream.is_open());

  char ReadBuffer[5];
  InStream.read(ReadBuffer, sizeof(ReadBuffer));

  for (size_t I = 0; I < sizeof(BinaryData); ++I) {
    EXPECT_EQ(ReadBuffer[I], BinaryData[I]);
  }

  InStream.close();
}

// Multiple Operations Test
TEST_F(VfsIoTest, MultipleOperations) {
  WasmEdge::FStream::OFStream OutStream(&TestEnv, "output.txt");
  EXPECT_TRUE(OutStream.is_open());

  OutStream << "First line\n";
  OutStream.put('X');
  OutStream.write(" Second part", 12);
  OutStream.close();

  WasmEdge::FStream::IFStream InStream(&TestEnv, "output.txt");
  EXPECT_TRUE(InStream.is_open());

  std::string FirstLine;
  InStream.getline(FirstLine);
  EXPECT_EQ(FirstLine, "First line");

  char NextChar = InStream.get();
  EXPECT_EQ(NextChar, 'X');

  char Buffer[13];
  InStream.read(Buffer, 12);
  Buffer[12] = '\0';
  EXPECT_STREQ(Buffer, " Second part");

  InStream.close();
}

// Non-WASI mode tests
TEST_F(VfsIoTest, NonWasiMode) {
  WasmEdge::FStream::OFStream OutStream(nullptr, "output.txt");
  EXPECT_TRUE(OutStream.is_open());

  OutStream << "Non-WASI test";
  OutStream.close();

  WasmEdge::FStream::IFStream InStream(nullptr, "output.txt");
  EXPECT_TRUE(InStream.is_open());

  std::string Content;
  InStream.getline(Content);
  EXPECT_EQ(Content, "Non-WASI test");

  InStream.close();
}
