// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_guest_file.h"

#include "common/spdlog.h"
#include "common/types.h"
#include "host/filesystem.h"
#include "host/wasi/environ.h"
#include "host/wasi/wasimodule.h"
#include "runtime/callingframe.h"
#include "runtime/instance/module.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/ostream_sink.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <system_error>
#include <thread>
#include <unistd.h>
#include <vector>

#if defined(__linux__)
#include <sys/mount.h>
#elif defined(__APPLE__)
#include <sys/param.h>
#endif

namespace {

using namespace std::literals;
using namespace WasmEdge::Host::StableDiffusion;
namespace Host = WasmEdge::Host;
namespace Runtime = WasmEdge::Runtime;

// Returns the change time of a file, or nullopt if stat fails.
std::optional<detail::Stamp> changeTimeAt(const std::filesystem::path &File) {
  struct stat St{};
  if (::stat(File.c_str(), &St) != 0) {
    return std::nullopt;
  }
  return detail::changeTimeOf(St);
}

// Returns true when a refusal line ends with the remedy.
bool endsWithTheRemedy(std::string_view Line) {
  constexpr auto kRemedy = "; preopen the file's directory with :readonly, "
                           "outside every directory the guest can write\n"sv;
  return Line.size() >= kRemedy.size() &&
         Line.substr(Line.size() - kRemedy.size()) == kRemedy;
}

class CallerModule : public Runtime::Instance::ModuleInstance {
public:
  using ModuleInstance::ModuleInstance;
  using ModuleInstance::setWASIModule;
};

// A filesystem that is not a WASI environment. It counts the open requests.
class CountingFileSystem : public Host::FileSystem {
public:
  Host::WASI::WasiExpect<std::shared_ptr<Host::File>>
  open(std::string_view, __wasi_oflags_t, __wasi_rights_t,
       __wasi_fdflags_t) const noexcept override {
    ++Opens;
    return Host::WASI::WasiUnexpect(__WASI_ERRNO_NOENT);
  }
  Host::WASI::WasiExpect<void>
  createDirectory(std::string_view) const noexcept override {
    return Host::WASI::WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  }
  mutable int Opens = 0;
};

// A module in the place of the WASI module. It provides that filesystem.
class ForeignWasiModule : public Runtime::Instance::ModuleInstance {
public:
  ForeignWasiModule() : ModuleInstance("wasi_snapshot_preview1") {
    setFileSystem(&FS);
  }
  CountingFileSystem FS;
};

class SDGuestFileTest : public ::testing::Test {
protected:
  void SetUp() override {
    namespace fs = std::filesystem;
    LoggerBefore = spdlog::default_logger();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "sd-guest-file-quiet",
        std::make_shared<spdlog::sinks::null_sink_mt>()));
    Root = fs::canonical(fs::temp_directory_path()) /
           ("wasmedge-sd-gf-" + std::to_string(::getpid()));
    fs::remove_all(Root);
    fs::create_directories(Root / "models" / "sub");
    fs::create_directories(Root / "work");
    std::ofstream(Root / "models" / "m.bin", std::ios::binary) << "MODEL";
    std::ofstream(Root / "work" / "secret", std::ios::binary) << "SECRET";
    fs::create_symlink("m.bin", Root / "models" / "alias");
    fs::create_symlink("../work/secret", Root / "models" / "out");
  }
  void TearDown() override {
    std::error_code Error;
    std::filesystem::remove_all(Root, Error);
    spdlog::set_default_logger(LoggerBefore);
    spdlog::drop("sd-guest-file-quiet");
  }

  void start(std::vector<std::string> Dirs) {
    Wasi.init(Dirs, "test", {}, {});
    Caller.setWASIModule(&Wasi);
  }
  // The WASI environment of the fixture, mutable, to do what a guest could do.
  Host::WASI::Environ &env() {
    return const_cast<Host::WASI::Environ &>(*Wasi.getEnv());
  }
  static std::string ro(const std::string &Guest, const std::string &Dir) {
    return Guest + ":" + Dir + ":readonly";
  }
  static std::string rw(const std::string &Guest, const std::string &Dir) {
    return Guest + ":" + Dir;
  }
  __wasi_fd_t preopenFd(std::string_view Name) {
    const auto End = env().getInitialDescriptors().size();
    for (__wasi_fd_t Fd = 3; static_cast<size_t>(Fd) < End; ++Fd) {
      __wasi_prestat_t Stat{};
      if (!env().fdPrestatGet(Fd, Stat)) {
        continue;
      }
      std::vector<uint8_t> Buffer(
          WasmEdge::EndianValue(Stat.u.dir.pr_name_len).le());
      if (env().fdPrestatDirName(Fd, Buffer) &&
          std::string_view(reinterpret_cast<const char *>(Buffer.data()),
                           Buffer.size()) == Name) {
        return Fd;
      }
    }
    ADD_FAILURE() << "no preopen named " << Name;
    return -1;
  }
  static std::ptrdiff_t openDescriptors() {
    return std::distance(std::filesystem::directory_iterator("/dev/fd"),
                         std::filesystem::directory_iterator());
  }
  // A creation time after all files of the test have settled.
  static std::chrono::system_clock::time_point later() {
    return std::chrono::system_clock::now() + std::chrono::hours(1);
  }
  static std::chrono::system_clock::time_point earlier() {
    return std::chrono::system_clock::now() - std::chrono::hours(1);
  }
  // Waits until no file of Files counts as changed since the current time.
  // Returns that time.
  static std::chrono::system_clock::time_point
  whenSettled(std::initializer_list<std::filesystem::path> Files) {
    const auto GiveUp =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (true) {
      const auto Now = std::chrono::system_clock::now();
      const bool Settled = std::all_of(
          Files.begin(), Files.end(), [Now](const std::filesystem::path &File) {
            const auto Changed = changeTimeAt(File);
            return Changed &&
                   !detail::changedSince(*Changed, detail::stampOf(Now));
          });
      if (Settled) {
        return Now;
      }
      if (std::chrono::steady_clock::now() > GiveUp) {
        ADD_FAILURE() << "the files did not settle within 5 seconds";
        return Now;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  std::shared_ptr<spdlog::logger> LoggerBefore;
  std::filesystem::path Root;
  Host::WasiModule Wasi;
  CallerModule Caller{""};
  Runtime::CallingFrame Frame{nullptr, &Caller};
};

TEST_F(SDGuestFileTest, LocatesAFileThroughAReadOnlyPreopen) {
  start({ro("/models", (Root / "models").string())});
  const auto Before = openDescriptors();
  {
    auto F = detail::locateGuestFile(env(), "/models/m.bin");
    ASSERT_TRUE(F);
    EXPECT_EQ(F->HostPath, (Root / "models" / "m.bin").string());
    EXPECT_EQ(F->Links, 1U);
    char Buf[8] = {};
    EXPECT_EQ(::pread(F->Fd.get(), Buf, 5, 0), 5);
    EXPECT_STREQ(Buf, "MODEL");
  }
  EXPECT_EQ(openDescriptors(), Before);
}

TEST_F(SDGuestFileTest, AcceptsTheSpellingsTheGuestsOpenAccepts) {
  start({ro("/models", (Root / "models").string())});
  const auto Want = (Root / "models" / "m.bin").string();
  for (const auto Path :
       {"./models/m.bin"sv, "models//m.bin"sv, "/models/./m.bin"sv,
        "/models/sub/../m.bin"sv, "/models/alias"sv}) {
    auto F = detail::locateGuestFile(env(), Path);
    ASSERT_TRUE(F) << Path;
    EXPECT_EQ(F->HostPath, Want) << Path;
  }
  for (const auto Path :
       {"/models/../models/m.bin"sv, "/models/m.bin/"sv, "/models/alias/"sv,
        "/models/m.bin\0x"sv, "/models\0/m.bin"sv}) {
    auto F = detail::locateGuestFile(env(), Path);
    ASSERT_FALSE(F) << Path;
    EXPECT_EQ(F.error(), ErrNo::LoadFailed) << Path;
  }
}

TEST_F(SDGuestFileTest, FailsOutsideThePreopensAndOnWhatIsNotAFile) {
  start({ro("/models", (Root / "models").string())});
  for (const auto Path :
       {"/other/x"sv, "/models/missing"sv, "/models"sv, "/models/sub"sv,
        "/models/out"sv, ""sv, "/models/../work/secret"sv}) {
    auto F = detail::locateGuestFile(env(), Path);
    ASSERT_FALSE(F) << Path;
    EXPECT_EQ(F.error(), ErrNo::LoadFailed) << Path;
  }
}

TEST_F(SDGuestFileTest, FindsAFileUnderEachOfTwoPreopens) {
  start({rw("/b", (Root / "work").string()),
         ro("/a", (Root / "models").string())});
  auto A = detail::locateGuestFile(env(), "/a/m.bin");
  auto B = detail::locateGuestFile(env(), "/b/secret");
  ASSERT_TRUE(A);
  ASSERT_TRUE(B);
  EXPECT_EQ(A->HostPath, (Root / "models" / "m.bin").string());
  EXPECT_EQ(B->HostPath, (Root / "work" / "secret").string());
}

TEST_F(SDGuestFileTest, APreopenNameMatchesWholeComponentsOnly) {
  std::ofstream(Root / "models" / "sub" / "f", std::ios::binary) << "F";
  start({ro("/models", (Root / "models").string())});
  EXPECT_TRUE(detail::locateGuestFile(env(), "/models/sub/f"));
  const auto R = detail::locateGuestFile(env(), "/modelssub/f");
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::LoadFailed);
}

TEST_F(SDGuestFileTest, APreopenTheGuestRenumberedStillMatches) {
  start({ro("/models", (Root / "models").string())});
  ASSERT_TRUE(env().fdRenumber(preopenFd("models"), 0));
  const auto F = detail::locateGuestFile(env(), "/models/m.bin");
  ASSERT_TRUE(F);
  EXPECT_EQ(F->HostPath, (Root / "models" / "m.bin").string());
}

TEST_F(SDGuestFileTest, ADirectoryTheGuestOpenedIsNoPreopen) {
  std::ofstream(Root / "models" / "sub" / "f", std::ios::binary) << "F";
  start({ro("/models", (Root / "models").string())});
  const auto Models = preopenFd("models");
  const auto Sub = env().pathOpen(
      Models, "sub", static_cast<__wasi_lookupflags_t>(0),
      __WASI_OFLAGS_DIRECTORY, __WASI_RIGHTS_PATH_OPEN | __WASI_RIGHTS_FD_READ,
      __WASI_RIGHTS_FD_READ, static_cast<__wasi_fdflags_t>(0));
  ASSERT_TRUE(Sub);
  ASSERT_TRUE(env().fdRenumber(*Sub, Models));
  for (const auto Path : {"/models/f"sv, "/models/m.bin"sv, "/f"sv}) {
    const auto R = detail::locateGuestFile(env(), Path);
    ASSERT_FALSE(R) << Path;
    EXPECT_EQ(R.error(), ErrNo::LoadFailed) << Path;
  }
}

TEST_F(SDGuestFileTest, AClosedPreopenLeavesThePathToAShorterOne) {
  std::filesystem::create_directories(Root / "work" / "models");
  std::ofstream(Root / "work" / "models" / "m.bin", std::ios::binary)
      << "OTHER";
  start({ro("/", (Root / "work").string()),
         ro("/models", (Root / "models").string())});
  {
    const auto F = detail::locateGuestFile(env(), "/models/m.bin");
    ASSERT_TRUE(F);
    EXPECT_EQ(F->HostPath, (Root / "models" / "m.bin").string());
  }
  ASSERT_TRUE(env().fdClose(preopenFd("models")));
  const auto F = detail::locateGuestFile(env(), "/models/m.bin");
  ASSERT_TRUE(F);
  EXPECT_EQ(F->HostPath, (Root / "work" / "models" / "m.bin").string());
}

TEST_F(SDGuestFileTest, GivesNothingThroughAPreopenTheGuestGaveUp) {
  start({ro("/models", (Root / "models").string()),
         ro("/second", (Root / "models").string())});
  ASSERT_TRUE(env().fdFdstatSetRights(preopenFd("second"),
                                      __WASI_RIGHTS_PATH_OPEN,
                                      static_cast<__wasi_rights_t>(0)));
  {
    const auto R = detail::locateGuestFile(env(), "/second/m.bin");
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error(), ErrNo::LoadFailed);
  }
  ASSERT_TRUE(env().fdClose(preopenFd("models")));
  {
    const auto R = detail::locateGuestFile(env(), "/models/m.bin");
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error(), ErrNo::LoadFailed);
  }
}

TEST_F(SDGuestFileTest, ARefusalLogsOneLineAndNoHostPath) {
  start({ro("/models", (Root / "models").string())});
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-guest-file", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  const auto R = detail::locateGuestFile(env(), "/models/out");
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-guest-file");
  ASSERT_FALSE(R);
  const auto Text = Out.str();
  EXPECT_EQ(std::count(Text.begin(), Text.end(), '\n'), 1);
  EXPECT_EQ(Text.rfind("error|[WasmEdge-StableDiffusion] ", 0), 0U);
  EXPECT_NE(Text.find("/models/out"), std::string::npos);
  EXPECT_EQ(Text.find("secret"), std::string::npos);
  EXPECT_EQ(Text.find(Root.string()), std::string::npos);
}

TEST_F(SDGuestFileTest, ARefusalKeepsTheGuestsLineBreaksOutOfTheLog) {
  start({ro("/models", (Root / "models").string())});
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-guest-file", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  const auto R = detail::locateGuestFile(env(), "/models/a\nerror|forged");
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-guest-file");
  ASSERT_FALSE(R);
  const auto Text = Out.str();
  EXPECT_EQ(std::count(Text.begin(), Text.end(), '\n'), 1);
}

TEST_F(SDGuestFileTest, ANamedPipeDoesNotBlockTheHost) {
  ASSERT_EQ(::mkfifo((Root / "models" / "pipe").c_str(), 0600), 0);
  start({ro("/models", (Root / "models").string())});
  const auto Before = openDescriptors();
  {
    const auto R = detail::locateGuestFile(env(), "/models/pipe");
    ASSERT_FALSE(R);
    EXPECT_EQ(R.error(), ErrNo::LoadFailed);
  }
  EXPECT_EQ(openDescriptors(), Before);
}

TEST_F(SDGuestFileTest, ACallerWithoutWasiHasNoEnvironment) {
  EXPECT_EQ(detail::wasiEnviron(Frame), nullptr);
  start({});
  EXPECT_EQ(detail::wasiEnviron(Frame), Wasi.getEnv());
}

TEST_F(SDGuestFileTest, AFilesystemThatIsNoWasiEnvironmentGivesNothing) {
  ForeignWasiModule Foreign;
  Caller.setWASIModule(&Foreign);
  ASSERT_EQ(Frame.getFileSystem(), &Foreign.FS);
  EXPECT_EQ(detail::wasiEnviron(Frame), nullptr);
  const auto R = resolveGuestFile(Frame, "/models/m.bin", later());
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::LoadFailed);
  EXPECT_EQ(Foreign.FS.Opens, 0);
}

TEST_F(SDGuestFileTest, ARefusalSaysWhyThePathWasNotOpened) {
  // Returns the line that the resolution of GuestPath logs. It must be one
  // refusal.
  const auto Capture = [this](std::string_view GuestPath) {
    std::ostringstream Out;
    auto Logger = std::make_shared<spdlog::logger>(
        "sd-guest-file", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
    Logger->set_pattern("%v");
    const auto Previous = spdlog::default_logger();
    spdlog::set_default_logger(Logger);
    const auto R = resolveGuestFile(Frame, GuestPath, later());
    spdlog::set_default_logger(Previous);
    spdlog::drop("sd-guest-file");
    EXPECT_FALSE(R) << GuestPath;
    if (!R) {
      EXPECT_EQ(R.error(), ErrNo::LoadFailed) << GuestPath;
    }
    const auto Text = Out.str();
    EXPECT_EQ(std::count(Text.begin(), Text.end(), '\n'), 1) << Text;
    return Text;
  };
  EXPECT_NE(Capture("/models/m.bin").find("imports no WASI"),
            std::string::npos);
  ForeignWasiModule Foreign;
  Caller.setWASIModule(&Foreign);
  EXPECT_NE(Capture("/models/m.bin").find("is not a WASI environment"),
            std::string::npos);
  start({ro("/models", (Root / "models").string())});
  EXPECT_NE(Capture("/other/x").find("no preopened directory"),
            std::string::npos);
  EXPECT_NE(Capture("/models/missing").find("does not open it"),
            std::string::npos);
}

TEST(SDGuestFileRuleTest, ClassifiesStarts) {
  using detail::isWritableStart;
  const auto None = static_cast<__wasi_rights_t>(0);
  EXPECT_FALSE(isWritableStart(
      __WASI_RIGHTS_PATH_OPEN | __WASI_RIGHTS_FD_READDIR |
          __WASI_RIGHTS_PATH_READLINK | __WASI_RIGHTS_PATH_FILESTAT_GET |
          __WASI_RIGHTS_FD_FILESTAT_GET,
      __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK | __WASI_RIGHTS_FD_SYNC |
          __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS));
  EXPECT_TRUE(isWritableStart(__WASI_RIGHTS_PATH_CREATE_FILE, None));
  EXPECT_TRUE(isWritableStart(None, __WASI_RIGHTS_FD_WRITE));
  EXPECT_TRUE(isWritableStart(__WASI_RIGHTS_PATH_FILESTAT_SET_TIMES, None));
  EXPECT_TRUE(isWritableStart(__WASI_RIGHTS_PATH_LINK_SOURCE, None));
}

TEST(SDGuestFileRuleTest, PathContainsComparesWholeComponents) {
  using detail::pathContains;
  EXPECT_TRUE(pathContains("/work", "/work/models/m"));
  EXPECT_TRUE(pathContains("/work/models", "/work/models/m"));
  EXPECT_TRUE(pathContains("/", "/m"));
  EXPECT_FALSE(pathContains("/work", "/workshop/m"));
  EXPECT_FALSE(pathContains("/work/m", "/work/m"));
  EXPECT_FALSE(pathContains("/work/models/sub", "/work/models/m"));
}

TEST(SDGuestFileRuleTest, TheMountTableShowsATreeMountedTwice) {
  using namespace detail;
  const auto Rows = parseMountInfo(
      "480 400 0:61 / / rw,relatime - overlay overlay rw\n"
      "526 480 0:35 /t/models /models ro,relatime - fuse.x /run/host rw\n"
      "528 480 0:35 /t /work rw,relatime shared:7 - fuse.x /run/host rw\n"
      "530 480 0:35 /t/out /out rw,relatime - fuse.x /run/host rw\n"
      "366 480 0:75 / /mnt/with\\040space rw - tmpfs none rw\n"
      "532 480 0:35 /t/models/m.safetensors /w2/m.safetensors rw - fuse.x "
      "/run/host rw\n"sv);
  ASSERT_TRUE(Rows);
  ASSERT_EQ(Rows->size(), 6U);
  EXPECT_EQ((*Rows)[4].Point, "/mnt/with space");
  auto Paths = aliasPaths(*Rows, 526, "/models/m.safetensors");
  ASSERT_TRUE(Paths);
  std::sort(Paths->begin(), Paths->end());
  EXPECT_EQ(*Paths, (std::vector<std::string>{"/models/m.safetensors",
                                              "/w2/m.safetensors",
                                              "/work/models/m.safetensors"}));
  auto Bound = aliasPaths(*Rows, 532, "/w2/m.safetensors");
  ASSERT_TRUE(Bound);
  std::sort(Bound->begin(), Bound->end());
  EXPECT_EQ(*Bound, *Paths);
  auto Out = aliasPaths(*Rows, 530, "/out/x.png");
  ASSERT_TRUE(Out);
  std::sort(Out->begin(), Out->end());
  EXPECT_EQ(*Out, (std::vector<std::string>{"/out/x.png", "/work/out/x.png"}));
  auto Plain = aliasPaths(*Rows, 480, "/etc/hosts");
  ASSERT_TRUE(Plain);
  EXPECT_EQ(*Plain, std::vector<std::string>{"/etc/hosts"});
  auto Spaced = aliasPaths(*Rows, 366, "/mnt/with space/m");
  ASSERT_TRUE(Spaced);
  EXPECT_EQ(*Spaced, std::vector<std::string>{"/mnt/with space/m"});
  EXPECT_FALSE(aliasPaths(*Rows, 999, "/models/m"));
  EXPECT_FALSE(aliasPaths(*Rows, 526, "/elsewhere/m"));
  EXPECT_FALSE(parseMountInfo("526 480 0:35 /t\n"sv));
  EXPECT_FALSE(parseMountInfo("x 480 0:35 / / rw - tmpfs none rw\n"sv));
}

TEST(SDGuestFileRuleTest, AChangeTimeIsComparedWithSlack) {
  using detail::changedSince;
  using detail::Stamp;
  const Stamp Since{1000, 500000000};
  EXPECT_FALSE(changedSince({999, 999999999}, Since));
  EXPECT_FALSE(changedSince({1000, 479999999}, Since));
  EXPECT_TRUE(changedSince({1000, 480000000}, Since));
  EXPECT_TRUE(changedSince({1000, 600000000}, Since));
  EXPECT_TRUE(changedSince({2000, 1}, Since));
  EXPECT_TRUE(changedSince({1000, 0}, Since));
  EXPECT_FALSE(changedSince({999, 0}, Since));
  const Stamp EarlyInItsSecond{1000, 10000000};
  EXPECT_TRUE(changedSince({999, 990000000}, EarlyInItsSecond));
  EXPECT_FALSE(changedSince({999, 989999999}, EarlyInItsSecond));
  EXPECT_TRUE(changedSince({999, 0}, EarlyInItsSecond));
  EXPECT_FALSE(changedSince({998, 0}, EarlyInItsSecond));
}

TEST(SDGuestFileRuleTest, AMountBelowADirectoryReachesItsFileSystem) {
  using namespace detail;
  const auto Rows =
      parseMountInfo("480 400 0:61 / / rw,relatime - overlay overlay rw\n"
                     "526 480 0:35 / /fs2 rw - tmpfs none rw\n"
                     "527 480 0:35 /cache /w/cache rw - tmpfs none rw\n"
                     "528 480 0:77 / /work2/cache rw - tmpfs none rw\n"sv);
  ASSERT_TRUE(Rows);
  EXPECT_EQ(reachesFileSystem(*Rows, 480, "/w", "0:35"), std::optional(true));
  EXPECT_EQ(reachesFileSystem(*Rows, 480, "/w/cache", "0:35"),
            std::optional(true));
  EXPECT_EQ(reachesFileSystem(*Rows, 480, "/", "0:35"), std::optional(true));
  EXPECT_EQ(reachesFileSystem(*Rows, 527, "/w/cache/sub", "0:35"),
            std::optional(true));
  EXPECT_EQ(reachesFileSystem(*Rows, 480, "/x", "0:35"), std::optional(false));
  EXPECT_EQ(reachesFileSystem(*Rows, 480, "/w", "0:99"), std::optional(false));
  EXPECT_EQ(reachesFileSystem(*Rows, 480, "/work", "0:77"),
            std::optional(false));
  EXPECT_EQ(reachesFileSystem(*Rows, 999, "/w", "0:35"), std::nullopt);
}

TEST(SDGuestFileRuleTest, AnAutomountPointBelowADirectoryIsSeen) {
  using namespace detail;
  const auto Rows = parseMountInfo(
      "480 400 0:61 / / rw,relatime - overlay overlay rw\n"
      "600 480 0:50 / /home/u/net rw,relatime - autofs /etc/auto.net "
      "rw,fd=7,pgrp=1,timeout=300\n"
      "601 480 0:51 / /efi rw,relatime - autofs systemd-1 rw,fd=40,direct\n"
      "602 601 8:1 / /efi rw,relatime - vfat /dev/sda1 rw\n"
      "603 600 0:52 / /home/u/net/server rw - nfs4 server:/ rw\n"sv);
  ASSERT_TRUE(Rows);
  EXPECT_EQ((*Rows)[1].Type, "autofs");
  EXPECT_EQ((*Rows)[3].Type, "vfat");
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 480, "/home/u"), std::optional(true));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 480, "/"), std::optional(true));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 600, "/home/u/net"),
            std::optional(true));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 602, "/efi"), std::optional(true));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 603, "/home/u/net/server/dir"),
            std::optional(false));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 480, "/home/v"), std::optional(false));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 480, "/home/unet"),
            std::optional(false));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 999, "/home/u"), std::nullopt);
  EXPECT_FALSE(parseMountInfo("480 400 0:61 / / rw -\n"sv));
  EXPECT_EQ(mountsOnDemandBelow(*Rows, 600, "/home/u/net/sub"),
            std::optional(true));
  EXPECT_FALSE(parseMountInfo("480 400 0:61 / / rw - \n"sv));
}

TEST(SDGuestFileRuleTest, AMomentBecomesAStamp) {
  using detail::stampOf;
  const std::chrono::system_clock::time_point Epoch{};
  const auto After = stampOf(Epoch + std::chrono::seconds(1000) +
                             std::chrono::microseconds(250));
  EXPECT_EQ(After.Sec, 1000);
  EXPECT_EQ(After.Nsec, 250000);
  const auto Before = stampOf(Epoch - std::chrono::microseconds(250));
  EXPECT_EQ(Before.Sec, -1);
  EXPECT_EQ(Before.Nsec, 999750000);
}

TEST_F(SDGuestFileTest, AcceptsAFileNoWritableStartContains) {
  start({ro("/models", (Root / "models").string()),
         rw("/work", (Root / "work").string()),
         rw("/sub", (Root / "models" / "sub").string())});
  auto Path = resolveGuestFile(Frame, "/models/m.bin", later());
  ASSERT_TRUE(Path);
  EXPECT_EQ(*Path, (Root / "models" / "m.bin").string());
}

TEST_F(SDGuestFileTest, RefusesAFileUnderAWritableStart) {
  start({rw("/models", (Root / "models").string())});
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, RefusesWhenAWritableStartContainsTheReadOnlyOne) {
  start({ro("/models", (Root / "models").string()), rw("/all", Root.string())});
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, RefusesTheSameDirectoryPreopenedTwice) {
  start({ro("/models", (Root / "models").string()),
         rw("/same", (Root / "models").string())});
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, WhatTheGuestDoesToItsDescriptorsChangesNothing) {
  start({ro("/models", (Root / "models").string()), rw("/all", Root.string())});
  const auto All = preopenFd("all");
  ASSERT_TRUE(env().fdFdstatSetRights(All, __WASI_RIGHTS_PATH_OPEN,
                                      __WASI_RIGHTS_FD_READ));
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
  ASSERT_TRUE(env().fdRenumber(All, 0));
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
  ASSERT_TRUE(env().fdClose(0));
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, ASecondNameCountsOnlyWhenTheGuestCanWrite) {
  std::filesystem::create_hard_link(Root / "models" / "m.bin",
                                    Root / "models" / "second");
  start({ro("/models", (Root / "models").string())});
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", later()));
  WasmEdge::Host::WasiModule Other;
  Other.init(std::vector<std::string>{ro("/models", (Root / "models").string()),
                                      rw("/work", (Root / "work").string())},
             "test", {}, {});
  Caller.setWASIModule(&Other);
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, AStandardStreamOnTheFileRefusesIt) {
  const detail::UniqueFd Out(
      ::open((Root / "models" / "m.bin").c_str(), O_WRONLY));
  ASSERT_GE(Out.get(), 0);
  ASSERT_TRUE(Wasi.initWithFds(
      std::vector<std::string>{ro("/models", (Root / "models").string())},
      "test", {}, {}, 0, Out.get(), 2));
  Caller.setWASIModule(&Wasi);
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, AClosedStreamAllowsAFileThatDidNotChange) {
  const detail::UniqueFd Out(
      ::open((Root / "models" / "m.bin").c_str(), O_WRONLY));
  ASSERT_GE(Out.get(), 0);
  ASSERT_TRUE(Wasi.initWithFds(
      std::vector<std::string>{ro("/models", (Root / "models").string())},
      "test", {}, {}, 0, Out.get(), 2));
  Caller.setWASIModule(&Wasi);
  ASSERT_TRUE(env().fdClose(1));
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", later()));
}

TEST_F(SDGuestFileTest, AClosedStreamThatChangedTheFileStillRefusesIt) {
  const detail::UniqueFd Out(
      ::open((Root / "models" / "m.bin").c_str(), O_WRONLY));
  ASSERT_GE(Out.get(), 0);
  ASSERT_TRUE(Wasi.initWithFds(
      std::vector<std::string>{ro("/models", (Root / "models").string())},
      "test", {}, {}, 0, Out.get(), 2));
  Caller.setWASIModule(&Wasi);
  const auto Created = whenSettled({Root / "models" / "m.bin"});
  const auto File = detail::locateGuestFile(env(), "/models/m.bin");
  ASSERT_TRUE(File);
  const std::array<uint8_t, 1> Byte{'X'};
  std::array<WasmEdge::Span<const uint8_t>, 1> Data{Byte};
  __wasi_size_t Written = 0;
  ASSERT_TRUE(env().fdWrite(1, Data, Written));
  ASSERT_TRUE(env().fdClose(1));
  EXPECT_EQ(detail::checkNeverWritable(env(), *File, "/models/m.bin",
                                       detail::stampOf(Created)),
            ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, ResolvesTheFilesOfAConfigurationAndARequest) {
  start({ro("/models", (Root / "models").string()),
         rw("/work", (Root / "work").string())});
  SessionConfig Config;
  Config.ModelPath = "/models/m.bin";
  Config.Embeddings = {{"e", "/models/alias"}};
  auto Files = resolveSessionFiles(Frame, Config, later());
  ASSERT_TRUE(Files);
  EXPECT_EQ(Files->Model, (Root / "models" / "m.bin").string());
  EXPECT_TRUE(Files->Vae.empty());
  ASSERT_EQ(Files->Embeddings.size(), 1U);
  EXPECT_EQ(Files->Embeddings[0], (Root / "models" / "m.bin").string());
  Config.VaePath = "/work/secret";
  EXPECT_EQ(resolveSessionFiles(Frame, Config, later()).error(),
            ErrNo::NotReadOnly);
  Config.VaePath = "/models/missing";
  EXPECT_EQ(resolveSessionFiles(Frame, Config, later()).error(),
            ErrNo::LoadFailed);
  GenerationRequest Req;
  Req.Loras = {{"/models/m.bin", 1.0F}, {"/work/secret", 1.0F}};
  EXPECT_EQ(resolveRequestFiles(Frame, Req, later()).error(),
            ErrNo::NotReadOnly);
  Req.Loras.pop_back();
  ASSERT_TRUE(resolveRequestFiles(Frame, Req, later()));
  Caller.setWASIModule(nullptr);
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::LoadFailed);
}

TEST_F(SDGuestFileTest, AFileChangedSinceTheModuleWasCreatedIsRefused) {
  start({ro("/models", (Root / "models").string()),
         rw("/work", (Root / "work").string())});
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", later()));
  const auto R = resolveGuestFile(Frame, "/models/m.bin", earlier());
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
  WasmEdge::Host::WasiModule Other;
  Other.init(
      std::vector<std::string>{ro("/models", (Root / "models").string())},
      "test", {}, {});
  Caller.setWASIModule(&Other);
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", earlier()));
}

TEST_F(SDGuestFileTest, ASecondNameTheGuestDroppedStillCounts) {
  {
    std::ofstream F(Root / "models" / "n.bin");
    F << "OTHER";
  }
  std::filesystem::create_hard_link(Root / "models" / "m.bin",
                                    Root / "work" / "second");
  start({ro("/models", (Root / "models").string()),
         rw("/work", (Root / "work").string())});
  const auto Created =
      whenSettled({Root / "models" / "m.bin", Root / "models" / "n.bin"});
  const auto Kept = env().pathOpen(
      preopenFd("work"), "second", static_cast<__wasi_lookupflags_t>(0),
      static_cast<__wasi_oflags_t>(0), __WASI_RIGHTS_FD_WRITE,
      static_cast<__wasi_rights_t>(0), static_cast<__wasi_fdflags_t>(0));
  ASSERT_TRUE(Kept);
  ASSERT_TRUE(env().pathUnlinkFile(preopenFd("work"), "second"));
  ASSERT_EQ(std::filesystem::hard_link_count(Root / "models" / "m.bin"), 1U);
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/n.bin", Created));
  const auto R = resolveGuestFile(Frame, "/models/m.bin", Created);
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
}

TEST_F(SDGuestFileTest, ANotReadOnlyRefusalNamesTheGuestsPathAndThePreopen) {
  start({ro("/models", (Root / "models").string()),
         rw("/zz-writable", Root.string())});
  std::ostringstream Out;
  auto Logger = std::make_shared<spdlog::logger>(
      "sd-guest-file", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
  Logger->set_pattern("%l|%v");
  const auto Previous = spdlog::default_logger();
  spdlog::set_default_logger(Logger);
  const auto R = resolveGuestFile(Frame, "/models/m.bin", later());
  spdlog::set_default_logger(Previous);
  spdlog::drop("sd-guest-file");
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
  const auto Text = Out.str();
  EXPECT_EQ(std::count(Text.begin(), Text.end(), '\n'), 1);
  EXPECT_EQ(Text.rfind("error|[WasmEdge-StableDiffusion] ", 0), 0U);
  EXPECT_NE(Text.find("/models/m.bin"), std::string::npos);
  EXPECT_NE(Text.find("zz-writable"), std::string::npos);
  EXPECT_EQ(Text.find(Root.string()), std::string::npos);
  EXPECT_TRUE(endsWithTheRemedy(Text)) << Text;
}

TEST_F(SDGuestFileTest, ARefusalSaysWhichRuleRefused) {
  // Returns the line that the resolution of GuestPath logs. It must be one
  // refusal.
  const auto Capture = [this](std::string_view GuestPath,
                              std::chrono::system_clock::time_point CreatedAt) {
    std::ostringstream Out;
    auto Logger = std::make_shared<spdlog::logger>(
        "sd-guest-file", std::make_shared<spdlog::sinks::ostream_sink_mt>(Out));
    Logger->set_pattern("%v");
    const auto Previous = spdlog::default_logger();
    spdlog::set_default_logger(Logger);
    const auto R = resolveGuestFile(Frame, GuestPath, CreatedAt);
    spdlog::set_default_logger(Previous);
    spdlog::drop("sd-guest-file");
    EXPECT_FALSE(R) << GuestPath;
    if (!R) {
      EXPECT_EQ(R.error(), ErrNo::NotReadOnly) << GuestPath;
    }
    const auto Text = Out.str();
    EXPECT_EQ(std::count(Text.begin(), Text.end(), '\n'), 1) << Text;
    EXPECT_NE(Text.find(GuestPath), std::string::npos) << Text;
    EXPECT_TRUE(endsWithTheRemedy(Text)) << Text;
    return Text;
  };
  start({rw("/models", (Root / "models").string())});
  EXPECT_NE(Capture("/models/m.bin", later()).find("holds it"),
            std::string::npos);
  WasmEdge::Host::WasiModule Other;
  Other.init(std::vector<std::string>{ro("/models", (Root / "models").string()),
                                      rw("/work", (Root / "work").string())},
             "test", {}, {});
  Caller.setWASIModule(&Other);
  EXPECT_NE(Capture("/models/m.bin", earlier()).find("change time"),
            std::string::npos);
  std::filesystem::create_hard_link(Root / "models" / "m.bin",
                                    Root / "models" / "second");
  EXPECT_NE(Capture("/models/m.bin", later()).find("more than one name"),
            std::string::npos);
}

#if defined(__APPLE__)
// Opens the root of the data volume as a preopen, first writable, then
// read-only. Writes nothing.
TEST_F(SDGuestFileTest, RefusesAFileBelowAWritableDataVolume) {
  {
    const detail::UniqueFd Dir(::open(Root.c_str(), O_RDONLY | O_DIRECTORY));
    std::array<char, MAXPATHLEN> Raw{};
    if (Dir.get() < 0 ||
        ::fcntl(Dir.get(), F_GETPATH_NOFIRMLINK, Raw.data()) == -1 ||
        std::string_view(Raw.data()).rfind("/System/Volumes/Data/", 0) != 0) {
      GTEST_SKIP() << "the test directory is not on the data volume";
    }
  }
  start({ro("/models", (Root / "models").string()),
         rw("/data", "/System/Volumes/Data")});
  const auto R = resolveGuestFile(Frame, "/models/m.bin", later());
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
  WasmEdge::Host::WasiModule Other;
  Other.init(std::vector<std::string>{ro("/models", (Root / "models").string()),
                                      ro("/data", "/System/Volumes/Data")},
             "test", {}, {});
  Caller.setWASIModule(&Other);
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", later()));
}
#endif

// Needs mounts that CI cannot create. Skipped unless both variables name mount
// points of one tree: a read-only mount of the directory with m.bin, and a
// writable mount of its parent. A privileged container with two bind mounts can
// provide them.
TEST_F(SDGuestFileTest, RefusesATreeMountedTwice) {
  const char *Ro = std::getenv("WASMEDGE_SD_ALIAS_RO");
  const char *Rw = std::getenv("WASMEDGE_SD_ALIAS_RW");
  if (Ro == nullptr || Rw == nullptr) {
    GTEST_SKIP() << "no aliased mounts prepared";
  }
  start({ro("/models", Ro), rw("/work", Rw)});
  EXPECT_EQ(resolveGuestFile(Frame, "/models/m.bin", later()).error(),
            ErrNo::NotReadOnly);
}

// Needs mounts that CI cannot create. Skipped unless both variables are set.
// WASMEDGE_SD_NESTED_RO names a read-only directory with m.bin. The file has a
// second name elsewhere on its file system. WASMEDGE_SD_NESTED_RW names a
// writable directory on another file system. A mount of the first file system
// below it shows the second name.
TEST_F(SDGuestFileTest, RefusesASecondNameBehindAMountBelowAWritableStart) {
  const char *Ro = std::getenv("WASMEDGE_SD_NESTED_RO");
  const char *Rw = std::getenv("WASMEDGE_SD_NESTED_RW");
  if (Ro == nullptr || Rw == nullptr) {
    GTEST_SKIP() << "no nested mount prepared";
  }
  start({ro("/models", Ro), rw("/work", Rw)});
  const auto R = resolveGuestFile(Frame, "/models/m.bin", later());
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
  WasmEdge::Host::WasiModule Other;
  Other.init(std::vector<std::string>{ro("/models", Ro),
                                      rw("/work", (Root / "work").string())},
             "test", {}, {});
  Caller.setWASIModule(&Other);
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", later()));
}

// Needs a mount that the test can remove, which CI cannot create. Skipped
// unless WASMEDGE_SD_DETACH is "TREE:MOUNT": a directory with w3/models/m.bin,
// and a bind mount of that directory. The test detaches MOUNT lazily (Linux
// only). Then its writable start leaves the mount table.
TEST_F(SDGuestFileTest, RefusesWhenAWritableStartHasLeftTheMountTable) {
  const char *Detach = std::getenv("WASMEDGE_SD_DETACH");
  if (Detach == nullptr) {
    GTEST_SKIP() << "no mount to detach prepared";
  }
#if defined(__linux__)
  const std::string_view Spec(Detach);
  const auto Colon = Spec.find(':');
  ASSERT_NE(Colon, std::string_view::npos) << Spec;
  const std::string Tree(Spec.substr(0, Colon));
  const std::string Mount(Spec.substr(Colon + 1));
  start({ro("/models", Tree + "/w3/models"), rw("/work", Mount + "/w3")});
  ASSERT_EQ(::umount2(Mount.c_str(), MNT_DETACH), 0);
  const auto R = resolveGuestFile(Frame, "/models/m.bin", later());
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
#else
  GTEST_SKIP() << "a lazy unmount needs Linux";
#endif
}

// Needs an automount point that CI cannot create. Skipped unless both variables
// are set. WASMEDGE_SD_ONDEMAND_RO names a read-only directory with m.bin,
// which has one name. WASMEDGE_SD_ONDEMAND_RW names a writable directory
// without the file and with an autofs mount below it. Nothing must walk into
// that mount: with no daemon, a lookup there would block.
TEST_F(SDGuestFileTest, RefusesAWritableStartWithAnAutomountPointBelow) {
  const char *Ro = std::getenv("WASMEDGE_SD_ONDEMAND_RO");
  const char *Rw = std::getenv("WASMEDGE_SD_ONDEMAND_RW");
  if (Ro == nullptr || Rw == nullptr) {
    GTEST_SKIP() << "no automount point prepared";
  }
  start({ro("/models", Ro), rw("/work", Rw)});
  const auto R = resolveGuestFile(Frame, "/models/m.bin", later());
  ASSERT_FALSE(R);
  EXPECT_EQ(R.error(), ErrNo::NotReadOnly);
  WasmEdge::Host::WasiModule Other;
  Other.init(std::vector<std::string>{ro("/models", Ro),
                                      rw("/work", (Root / "work").string())},
             "test", {}, {});
  Caller.setWASIModule(&Other);
  EXPECT_TRUE(resolveGuestFile(Frame, "/models/m.bin", later()));
}

} // namespace
