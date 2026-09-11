// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- test/host/wasi/p3_filesystem.cpp - wasi:filesystem@0.3.1 host -----===//

#include "common/component_variant.h"
#include "common/filesystem.h"
#include "host/wasi/component/env.h"
#include "host/wasi/p3/filesystem.h"
#include "hostcall.h"
#include "runtime/component/wit.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace {

using namespace WasmEdge;
using namespace Host::WasiP3;
using HostTest::call;
namespace fs = std::filesystem;

// A fresh temporary directory preopened as the guest's root.
class WasiP3Filesystem : public testing::Test {
protected:
  void SetUp() override {
    Root = fs::temp_directory_path() / "wasmedge_p3_filesystem_test";
    fs::remove_all(Root);
    fs::create_directories(Root);
    const std::vector<std::string> Dirs{"/:" + Root.string()};
    Env.init(Dirs, "prog", {}, {});
    Insts = makeFilesystemInstances(Env);
    Types = static_cast<FilesystemTypesInstance *>(Insts[0].get());
    auto Res = call(*Insts[1], "get-directories", {});
    ASSERT_TRUE(Res);
    auto Dirs2 = Runtime::Component::Wit<
        std::vector<std::tuple<Runtime::Component::Own<Host::WASI::VINode>,
                               std::string>>>::from((*Res)[0]);
    ASSERT_EQ(Dirs2.size(), 1u);
    EXPECT_EQ(std::get<1>(Dirs2[0]), "/");
    RootRep = std::get<0>(Dirs2[0]).Rep;
  }
  void TearDown() override { fs::remove_all(Root); }

  ComponentValVariant self(uint64_t Rep) {
    return Runtime::Component::Wit<Descriptor>::into(Descriptor{Rep});
  }
  ComponentValVariant str(std::string S) {
    return Runtime::Component::Wit<std::string>::into(std::move(S));
  }
  ComponentValVariant pathFlags(uint32_t Bits) {
    return Runtime::Component::Wit<PathFlags>::into(
        static_cast<PathFlags>(Bits));
  }
  // open-at on Dir with the given open and descriptor flags.
  FsResult<Runtime::Component::Own<Host::WASI::VINode>>
  openAt(uint64_t Dir, std::string Path, uint32_t Open, uint32_t Flags) {
    auto Res = call(
        *Types, "[method]descriptor.open-at",
        {self(Dir), pathFlags(0), str(std::move(Path)),
         Runtime::Component::Wit<OpenFlags>::into(static_cast<OpenFlags>(Open)),
         Runtime::Component::Wit<DescriptorFlags>::into(
             static_cast<DescriptorFlags>(Flags))});
    if (!Res) {
      return Unexpected<FsError>(FsError());
    }
    return Runtime::Component::Wit<
        FsResult<Runtime::Component::Own<Host::WASI::VINode>>>::from((*Res)[0]);
  }
  FsResult<DescriptorStat> stat(uint64_t Rep) {
    auto Res = call(*Types, "[method]descriptor.stat", {self(Rep)});
    if (!Res) {
      return Unexpected<FsError>(FsError());
    }
    return Runtime::Component::Wit<FsResult<DescriptorStat>>::from((*Res)[0]);
  }
  FsStatus status(std::string_view Name,
                  std::vector<ComponentValVariant> Args) {
    auto Res = call(*Types, Name, std::move(Args));
    if (!Res) {
      return Unexpected<FsError>(FsError());
    }
    return Runtime::Component::Wit<FsStatus>::from((*Res)[0]);
  }

  fs::path Root;
  Host::WasiComponent::Env Env;
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Insts;
  FilesystemTypesInstance *Types = nullptr;
  uint64_t RootRep = 0;
};

constexpr uint32_t OpenCreate = 1;
constexpr uint32_t OpenDirectory = 2;
constexpr uint32_t OpenTruncate = 8;
constexpr uint32_t FlagRead = 1;
constexpr uint32_t FlagWrite = 2;

TEST_F(WasiP3Filesystem, InstancesAndTypes) {
  EXPECT_EQ(Insts[0]->getComponentName(), "wasi:filesystem/types@0.3.1");
  EXPECT_EQ(Insts[1]->getComponentName(), "wasi:filesystem/preopens@0.3.1");
  EXPECT_NE(Types->findTypeResource("descriptor"), nullptr);
  EXPECT_EQ(Insts[1]->findTypeResource("descriptor"),
            Types->findTypeResource("descriptor"));
  const auto *Err = Types->findType("error-code");
  ASSERT_NE(Err, nullptr);
  ASSERT_TRUE(Err->getDefValType().isVariantTy());
  ASSERT_EQ(Err->getDefValType().getVariant().Cases.size(), 37u);
  EXPECT_EQ(Err->getDefValType().getVariant().Cases[0].first, "access");
  EXPECT_EQ(Err->getDefValType().getVariant().Cases[36].first, "other");
  const auto *Stat = Types->findType("descriptor-stat");
  ASSERT_NE(Stat, nullptr);
  ASSERT_TRUE(Stat->getDefValType().isRecordTy());
  EXPECT_EQ(Stat->getDefValType().getRecord().LabelTypes.size(), 6u);
  auto *Open = Types->findFunction("[method]descriptor.open-at");
  ASSERT_NE(Open, nullptr);
  EXPECT_TRUE(Open->getFuncType().isAsync());
  EXPECT_EQ(Open->getFuncType().getParamList()[0].getLabel(), "self");
  EXPECT_EQ(Open->getFuncType().getParamList()[4].getLabel(), "flags");
  auto *Read = Types->findFunction("[method]descriptor.read-via-stream");
  ASSERT_NE(Read, nullptr);
  EXPECT_FALSE(Read->getFuncType().isAsync());
  EXPECT_EQ(Types->descriptors().size(), 1u);
}

TEST_F(WasiP3Filesystem, RootIsADirectory) {
  auto S = stat(RootRep);
  ASSERT_TRUE(S);
  EXPECT_EQ(S->Type.Case, DescriptorType::Directory);
  auto Ty = call(*Types, "[method]descriptor.get-type", {self(RootRep)});
  ASSERT_TRUE(Ty);
  auto T = Runtime::Component::Wit<FsResult<DescriptorType>>::from((*Ty)[0]);
  ASSERT_TRUE(T);
  EXPECT_EQ(T->Case, DescriptorType::Directory);
  auto Fl = call(*Types, "[method]descriptor.get-flags", {self(RootRep)});
  ASSERT_TRUE(Fl);
  auto F = Runtime::Component::Wit<FsResult<DescriptorFlags>>::from((*Fl)[0]);
  ASSERT_TRUE(F);
  EXPECT_NE(static_cast<uint32_t>(*F) &
                static_cast<uint32_t>(DescriptorFlags::MutateDirectory),
            0u);
}

TEST_F(WasiP3Filesystem, CreateStatResizeAndRemoveAFile) {
  auto File =
      openAt(RootRep, "a.txt", OpenCreate | OpenTruncate, FlagRead | FlagWrite);
  ASSERT_TRUE(File);
  EXPECT_EQ(Types->descriptors().size(), 2u);
  EXPECT_TRUE(fs::exists(Root / "a.txt"));
  auto S = stat(File->Rep);
  ASSERT_TRUE(S);
  EXPECT_EQ(S->Type.Case, DescriptorType::RegularFile);
  EXPECT_EQ(S->Size, 0u);
  EXPECT_EQ(S->LinkCount, 1u);
  EXPECT_TRUE(S->DataModification.has_value());
  EXPECT_GT(S->DataModification->Seconds, 1700000000);
  ASSERT_TRUE(status("[method]descriptor.set-size",
                     {self(File->Rep), ComponentValVariant{uint64_t(5)}}));
  EXPECT_EQ(stat(File->Rep)->Size, 5u);
  auto Fl = call(*Types, "[method]descriptor.get-flags", {self(File->Rep)});
  ASSERT_TRUE(Fl);
  auto F = Runtime::Component::Wit<FsResult<DescriptorFlags>>::from((*Fl)[0]);
  ASSERT_TRUE(F);
  EXPECT_EQ(static_cast<uint32_t>(*F) & (FlagRead | FlagWrite),
            FlagRead | FlagWrite);
  ASSERT_TRUE(status("[method]descriptor.sync", {self(File->Rep)}));
  ASSERT_TRUE(status("[method]descriptor.sync-data", {self(File->Rep)}));
  ASSERT_TRUE(
      status("[method]descriptor.advise",
             {self(File->Rep), ComponentValVariant{uint64_t(0)},
              ComponentValVariant{uint64_t(5)},
              Runtime::Component::Wit<Advice>::into(Advice::Sequential)}));
  auto St = call(*Types, "[method]descriptor.stat-at",
                 {self(RootRep), pathFlags(0), str("a.txt")});
  ASSERT_TRUE(St);
  auto At = Runtime::Component::Wit<FsResult<DescriptorStat>>::from((*St)[0]);
  ASSERT_TRUE(At);
  EXPECT_EQ(At->Size, 5u);
  Types->descriptors().remove(File->Rep);
  EXPECT_EQ(Types->descriptors().size(), 1u);
  ASSERT_TRUE(status("[method]descriptor.unlink-file-at",
                     {self(RootRep), str("a.txt")}));
  EXPECT_FALSE(fs::exists(Root / "a.txt"));
}

TEST_F(WasiP3Filesystem, MissingFilesReportNoEntry) {
  auto File = openAt(RootRep, "missing.txt", 0, FlagRead);
  ASSERT_FALSE(File);
  EXPECT_EQ(File.error().Code, Host::WasiComponent::FsErrorCode::NoEntry);
  auto S = stat(999);
  ASSERT_FALSE(S);
  EXPECT_EQ(S.error().Code, Host::WasiComponent::FsErrorCode::BadDescriptor);
  auto Res =
      call(*Types, "[method]descriptor.open-at",
           {self(RootRep), pathFlags(0), str("missing.txt"),
            Runtime::Component::Wit<OpenFlags>::into(static_cast<OpenFlags>(0)),
            Runtime::Component::Wit<DescriptorFlags>::into(
                static_cast<DescriptorFlags>(FlagRead))});
  ASSERT_TRUE(Res);
  // no-entry is the 20th case of the 0.3 variant.
  const auto &Val = Runtime::Component::WitVariant::value(
      std::get<ResultVal>(Runtime::Component::valComp((*Res)[0]).V)
          .Payload.value());
  EXPECT_EQ(Val.Case, 19u);
}

TEST_F(WasiP3Filesystem, DirectoriesLinksAndRenames) {
  ASSERT_TRUE(status("[method]descriptor.create-directory-at",
                     {self(RootRep), str("sub")}));
  EXPECT_TRUE(fs::is_directory(Root / "sub"));
  auto Sub = openAt(RootRep, "sub", OpenDirectory, FlagRead);
  ASSERT_TRUE(Sub) << static_cast<int>(Sub.error().Code);
  EXPECT_EQ(stat(Sub->Rep)->Type.Case, DescriptorType::Directory);

  auto File = openAt(Sub->Rep, "b.txt", OpenCreate, FlagRead | FlagWrite);
  ASSERT_TRUE(File);
  auto Sym = status("[method]descriptor.symlink-at",
                    {self(Sub->Rep), str("b.txt"), str("link")});
  ASSERT_TRUE(Sym) << static_cast<int>(Sym.error().Code);
  auto Link = call(*Types, "[method]descriptor.readlink-at",
                   {self(Sub->Rep), str("link")});
  ASSERT_TRUE(Link);
  auto Target =
      Runtime::Component::Wit<FsResult<std::string>>::from((*Link)[0]);
  ASSERT_TRUE(Target);
  EXPECT_EQ(*Target, "b.txt");
  auto Via = call(*Types, "[method]descriptor.stat-at",
                  {self(Sub->Rep), pathFlags(0), str("link")});
  ASSERT_TRUE(Via);
  EXPECT_EQ(Runtime::Component::Wit<FsResult<DescriptorStat>>::from((*Via)[0])
                ->Type.Case,
            DescriptorType::SymbolicLink);
  auto Followed = call(*Types, "[method]descriptor.stat-at",
                       {self(Sub->Rep), pathFlags(1), str("link")});
  ASSERT_TRUE(Followed);
  EXPECT_EQ(
      Runtime::Component::Wit<FsResult<DescriptorStat>>::from((*Followed)[0])
          ->Type.Case,
      DescriptorType::RegularFile);

  ASSERT_TRUE(status("[method]descriptor.link-at",
                     {self(Sub->Rep), pathFlags(0), str("b.txt"), self(RootRep),
                      str("c.txt")}));
  auto Other = openAt(RootRep, "c.txt", 0, FlagRead);
  ASSERT_TRUE(Other);
  auto Same = call(*Types, "[method]descriptor.is-same-object",
                   {self(File->Rep), self(Other->Rep)});
  ASSERT_TRUE(Same);
  EXPECT_TRUE(std::get<bool>((*Same)[0]));
  auto NotSame = call(*Types, "[method]descriptor.is-same-object",
                      {self(File->Rep), self(Sub->Rep)});
  ASSERT_TRUE(NotSame);
  EXPECT_FALSE(std::get<bool>((*NotSame)[0]));
  auto HashA =
      call(*Types, "[method]descriptor.metadata-hash", {self(File->Rep)});
  auto HashB = call(*Types, "[method]descriptor.metadata-hash-at",
                    {self(RootRep), pathFlags(0), str("c.txt")});
  ASSERT_TRUE(HashA);
  ASSERT_TRUE(HashB);
  auto HA =
      Runtime::Component::Wit<FsResult<MetadataHashValue>>::from((*HashA)[0]);
  auto HB =
      Runtime::Component::Wit<FsResult<MetadataHashValue>>::from((*HashB)[0]);
  ASSERT_TRUE(HA);
  ASSERT_TRUE(HB);
  EXPECT_EQ(HA->Lower, HB->Lower);
  EXPECT_EQ(HA->Upper, HB->Upper);
  EXPECT_EQ(stat(File->Rep)->LinkCount, 2u);

  ASSERT_TRUE(
      status("[method]descriptor.rename-at",
             {self(RootRep), str("c.txt"), self(Sub->Rep), str("d.txt")}));
  EXPECT_TRUE(fs::exists(Root / "sub" / "d.txt"));
  EXPECT_FALSE(fs::exists(Root / "c.txt"));

  NewTimestamp Now;
  Now.Case = NewTimestamp::Now;
  NewTimestamp Fixed;
  Fixed.Case = NewTimestamp::Timestamp;
  Fixed.Value = Instant{1700000000, 0};
  ASSERT_TRUE(status(
      "[method]descriptor.set-times",
      {self(File->Rep),
       Runtime::Component::Wit<NewTimestamp>::into(NewTimestamp(Now)),
       Runtime::Component::Wit<NewTimestamp>::into(NewTimestamp(Fixed))}));
  EXPECT_EQ(stat(File->Rep)->DataModification->Seconds, 1700000000);
  ASSERT_TRUE(
      status("[method]descriptor.set-times-at",
             {self(Sub->Rep), pathFlags(1), str("d.txt"),
              Runtime::Component::Wit<NewTimestamp>::into(NewTimestamp(Fixed)),
              Runtime::Component::Wit<NewTimestamp>::into(NewTimestamp(Now))}));
  EXPECT_GT(stat(File->Rep)->DataModification->Seconds, 1700000000);

  auto NotEmpty = status("[method]descriptor.remove-directory-at",
                         {self(RootRep), str("sub")});
  ASSERT_FALSE(NotEmpty);
  EXPECT_EQ(NotEmpty.error().Code, Host::WasiComponent::FsErrorCode::NotEmpty);
  ASSERT_TRUE(status("[method]descriptor.unlink-file-at",
                     {self(Sub->Rep), str("link")}));
  ASSERT_TRUE(status("[method]descriptor.unlink-file-at",
                     {self(Sub->Rep), str("b.txt")}));
  ASSERT_TRUE(status("[method]descriptor.unlink-file-at",
                     {self(Sub->Rep), str("d.txt")}));
  ASSERT_TRUE(status("[method]descriptor.remove-directory-at",
                     {self(RootRep), str("sub")}));
  EXPECT_FALSE(fs::exists(Root / "sub"));
}

TEST_F(WasiP3Filesystem, PathsCannotEscapeThePreopen) {
  auto Up = openAt(RootRep, "../outside.txt", OpenCreate, FlagWrite);
  ASSERT_FALSE(Up);
  EXPECT_EQ(Up.error().Code, Host::WasiComponent::FsErrorCode::NotPermitted);
  EXPECT_FALSE(fs::exists(Root.parent_path() / "outside.txt"));
}

} // namespace
