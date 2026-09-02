// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p2_filesystem.cpp - wasi:filesystem@0.2.12 host ----===//

#include "common/component_variant.h"
#include "common/filesystem.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p2/filesystem.h"
#include "host/wasi/p2/io.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace {

using namespace WasmEdge;
using namespace Host::WasiP2;
using HostTest::call;
namespace fs = std::filesystem;

class WasiP2Filesystem : public testing::Test {
protected:
  void SetUp() override {
    Root = fs::temp_directory_path() / "wasmedge_p2_filesystem_test";
    fs::remove_all(Root);
    fs::create_directories(Root);
    const std::vector<std::string> Dirs{"/:" + Root.string()};
    Env.init(Dirs, "prog", {}, {});
    Io = std::make_unique<IoHost>(Env);
    IoInsts = makeIoInstances(*Io);
    Insts = makeFilesystemInstances(Env, *Io);
    Types = static_cast<FilesystemTypesInstance *>(Insts[0].get());
    auto Res = call(*Insts[1], "get-directories", {});
    ASSERT_TRUE(Res);
    auto Dirs2 = Runtime::Component::Wit<
        std::vector<std::tuple<Runtime::Component::Own<Host::WASI::VINode>,
                               std::string>>>::from((*Res)[0]);
    ASSERT_EQ(Dirs2.size(), 1u);
    RootRep = std::get<0>(Dirs2[0]).Rep;
  }
  void TearDown() override { fs::remove_all(Root); }

  ComponentValVariant self(uint64_t Rep) {
    return Runtime::Component::Wit<Descriptor>::into(Descriptor{Rep});
  }
  ComponentValVariant str(std::string S) {
    return Runtime::Component::Wit<std::string>::into(std::move(S));
  }
  FsResult<Runtime::Component::Own<Host::WASI::VINode>>
  openAt(uint64_t Dir, std::string Path, uint32_t Open, uint32_t Flags) {
    auto Res = call(
        *Types, "[method]descriptor.open-at",
        {self(Dir), Runtime::Component::Wit<PathFlags>::into(PathFlags(0)),
         str(std::move(Path)),
         Runtime::Component::Wit<OpenFlags>::into(static_cast<OpenFlags>(Open)),
         Runtime::Component::Wit<DescriptorFlags>::into(
             static_cast<DescriptorFlags>(Flags))});
    if (!Res) {
      return Unexpected<FsErrorCode>(FsErrorCode::Io);
    }
    return Runtime::Component::Wit<
        FsResult<Runtime::Component::Own<Host::WASI::VINode>>>::from((*Res)[0]);
  }

  fs::path Root;
  Host::WasiComponent::Env Env;
  std::unique_ptr<IoHost> Io;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> IoInsts;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  FilesystemTypesInstance *Types = nullptr;
  uint64_t RootRep = 0;
};

TEST_F(WasiP2Filesystem, TypesAndSharedResources) {
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:filesystem/types@0.2.12");
  EXPECT_EQ(Insts[1]->getComponentName(), "wasi:filesystem/preopens@0.2.12");
  EXPECT_EQ(Types->findTypeResource("input-stream"), Io->InputType);
  EXPECT_EQ(Types->findTypeResource("error"), Io->ErrorType);
  EXPECT_NE(Types->findTypeResource("directory-entry-stream"), nullptr);
  const auto *Err = Types->findType("error-code");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isEnumTy());
  EXPECT_EQ(Err->getDefValType().getEnum().Labels.size(), 37u);
  EXPECT_EQ(Err->getDefValType().getEnum().Labels[1], "would-block");
  const auto *Ty = Types->findType("descriptor-type");
  ASSERT_TRUE(Ty->getDefValType().isEnumTy());
  EXPECT_EQ(Ty->getDefValType().getEnum().Labels[0], "unknown");
  auto *Stat = Types->findFunction("[method]descriptor.stat");
  ASSERT_NE(Stat, nullptr);
  EXPECT_FALSE(Stat->getFuncType().isAsync());
}

TEST_F(WasiP2Filesystem, ReadWriteAndStreams) {
  auto File = openAt(RootRep, "p2.txt", 1 | 8, 1 | 2);
  ASSERT_TRUE(File);
  auto Written = call(
      *Types, "[method]descriptor.write",
      {self(File->Rep),
       Runtime::Component::Wit<std::vector<uint8_t>>::into({'a', 'b', 'c'}),
       ComponentValVariant{uint64_t(0)}});
  ASSERT_TRUE(Written);
  EXPECT_EQ(*Runtime::Component::Wit<FsResult<uint64_t>>::from((*Written)[0]),
            3u);
  auto Read = call(*Types, "[method]descriptor.read",
                   {self(File->Rep), ComponentValVariant{uint64_t(10)},
                    ComponentValVariant{uint64_t(1)}});
  ASSERT_TRUE(Read);
  using Chunk = std::tuple<std::vector<uint8_t>, bool>;
  auto Got = Runtime::Component::Wit<FsResult<Chunk>>::from((*Read)[0]);
  ASSERT_TRUE(Got);
  EXPECT_EQ(std::get<0>(*Got), (std::vector<uint8_t>{'b', 'c'}));
  EXPECT_FALSE(std::get<1>(*Got));
  auto End = call(*Types, "[method]descriptor.read",
                  {self(File->Rep), ComponentValVariant{uint64_t(10)},
                   ComponentValVariant{uint64_t(3)}});
  ASSERT_TRUE(End);
  EXPECT_TRUE(
      std::get<1>(*Runtime::Component::Wit<FsResult<Chunk>>::from((*End)[0])));

  auto Append =
      call(*Types, "[method]descriptor.append-via-stream", {self(File->Rep)});
  ASSERT_TRUE(Append);
  auto Out = Runtime::Component::Wit<
      FsResult<Runtime::Component::Own<OutputStream>>>::from((*Append)[0]);
  ASSERT_TRUE(Out);
  auto Wrote =
      call(*IoInsts[2], "[method]output-stream.blocking-write-and-flush",
           {Runtime::Component::Wit<OutputBorrow>::into(OutputBorrow{Out->Rep}),
            Runtime::Component::Wit<std::vector<uint8_t>>::into({'d', 'e'})});
  ASSERT_TRUE(Wrote);
  EXPECT_TRUE(Runtime::Component::Wit<IoResult<Runtime::Component::Unit>>::from(
      (*Wrote)[0]));
  auto In = call(*Types, "[method]descriptor.read-via-stream",
                 {self(File->Rep), ComponentValVariant{uint64_t(1)}});
  ASSERT_TRUE(In);
  auto Input = Runtime::Component::Wit<
      FsResult<Runtime::Component::Own<InputStream>>>::from((*In)[0]);
  ASSERT_TRUE(Input);
  auto Rest =
      call(*IoInsts[2], "[method]input-stream.blocking-read",
           {Runtime::Component::Wit<InputBorrow>::into(InputBorrow{Input->Rep}),
            ComponentValVariant{uint64_t(100)}});
  ASSERT_TRUE(Rest);
  EXPECT_EQ(*Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from(
                (*Rest)[0]),
            (std::vector<uint8_t>{'b', 'c', 'd', 'e'}));
  auto Eof =
      call(*IoInsts[2], "[method]input-stream.blocking-read",
           {Runtime::Component::Wit<InputBorrow>::into(InputBorrow{Input->Rep}),
            ComponentValVariant{uint64_t(100)}});
  ASSERT_TRUE(Eof);
  EXPECT_TRUE(
      Runtime::Component::Wit<IoResult<std::vector<uint8_t>>>::from((*Eof)[0])
          .error()
          .Closed);
  std::ifstream Check(Root / "p2.txt", std::ios::binary);
  std::string Content((std::istreambuf_iterator<char>(Check)),
                      std::istreambuf_iterator<char>());
  EXPECT_EQ(Content, "abcde");
}

TEST_F(WasiP2Filesystem, DirectoryEntriesAndErrors) {
  std::ofstream(Root / "one.txt") << "1";
  std::ofstream(Root / "two.txt") << "22";
  fs::create_directory(Root / "sub");
  auto Res = call(*Types, "[method]descriptor.read-directory", {self(RootRep)});
  ASSERT_TRUE(Res);
  auto Stream = Runtime::Component::Wit<
      FsResult<Runtime::Component::Own<DirectoryEntryStream>>>::from((*Res)[0]);
  ASSERT_TRUE(Stream);
  std::vector<std::string> Names;
  while (true) {
    auto Entry =
        call(*Types, "[method]directory-entry-stream.read-directory-entry",
             {Runtime::Component::
                  Wit<Runtime::Component::Borrow<DirectoryEntryStream>>::into(
                      Runtime::Component::Borrow<DirectoryEntryStream>{
                          Stream->Rep})});
    ASSERT_TRUE(Entry);
    auto Next =
        Runtime::Component::Wit<FsResult<std::optional<DirectoryEntry>>>::from(
            (*Entry)[0]);
    ASSERT_TRUE(Next);
    if (!Next->has_value()) {
      break;
    }
    Names.push_back((*Next)->Name);
    if ((*Next)->Name == "sub") {
      EXPECT_EQ((*Next)->Type, DescriptorType::Directory);
    } else {
      EXPECT_EQ((*Next)->Type, DescriptorType::RegularFile);
    }
  }
  std::sort(Names.begin(), Names.end());
  EXPECT_EQ(Names, (std::vector<std::string>{"one.txt", "sub", "two.txt"}));

  auto Missing = openAt(RootRep, "missing", 0, 1);
  ASSERT_FALSE(Missing);
  EXPECT_EQ(Missing.error(), FsErrorCode::NoEntry);
  const uint64_t ErrRep = Io->errors().add(
      std::make_shared<IoError>(IoError{__WASI_ERRNO_NOENT, "x"}));
  auto Code =
      call(*Types, "filesystem-error-code",
           {Runtime::Component::Wit<ErrorBorrow>::into(ErrorBorrow{ErrRep})});
  ASSERT_TRUE(Code);
  auto Mapped =
      Runtime::Component::Wit<std::optional<FsErrorCode>>::from((*Code)[0]);
  ASSERT_TRUE(Mapped.has_value());
  EXPECT_EQ(*Mapped, FsErrorCode::NoEntry);
  auto Stat = call(*Types, "[method]descriptor.stat-at",
                   {self(RootRep),
                    Runtime::Component::Wit<PathFlags>::into(PathFlags(0)),
                    str("two.txt")});
  ASSERT_TRUE(Stat);
  auto S = Runtime::Component::Wit<FsResult<DescriptorStat>>::from((*Stat)[0]);
  ASSERT_TRUE(S);
  EXPECT_EQ(S->Size, 2u);
  EXPECT_EQ(S->Type, DescriptorType::RegularFile);
  EXPECT_GT(S->DataModification->Seconds, 1700000000u);
}

} // namespace
