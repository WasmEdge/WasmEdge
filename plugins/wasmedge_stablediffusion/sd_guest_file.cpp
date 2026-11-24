// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "sd_guest_file.h"

#include "sd_log.h"

#include "common/spdlog.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <cerrno>
#include <climits>
#elif defined(__APPLE__)
#include <cstring>
#include <sys/param.h>
#else
#error "The Stable Diffusion plug-in names guest files on Linux and macOS only."
#endif

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {
namespace detail {
namespace {

Unexpected<ErrNo> refuse(std::string_view GuestPath,
                         std::string_view Reason) noexcept {
  spdlog::error("[WasmEdge-StableDiffusion] guest file \"{}\": {}"sv,
                logText(GuestPath), Reason);
  return Unexpected<ErrNo>(ErrNo::LoadFailed);
}

// Returns the POSIX descriptor of a native handle, or -1 if there is none.
int posixFd(const WASI::WasiExpect<uint64_t> &Native) noexcept {
  if (!Native ||
      *Native > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
    return -1;
  }
  return static_cast<int>(*Native);
}

// The host descriptor of a node and the plug-in's own fstat result for it. The
// plug-in does not call VINode::isDirectory(): that function fills a cache in
// the node without a lock, and this code runs on several threads.
struct HostNode {
  int Fd;
  struct stat Stat;
};
std::optional<HostNode> hostNode(const WASI::VINode &Node) noexcept {
  HostNode Native{posixFd(Node.getNativeHandler()), {}};
  if (Native.Fd < 0 || ::fstat(Native.Fd, &Native.Stat) != 0) {
    return std::nullopt;
  }
  return Native;
}

// Returns the plug-in's own duplicate of the file at a guest path. The caller
// filesystem opens the file with the rights of the guest and inside the
// preopen. The file gets no descriptor in the guest table, where another guest
// thread could close or replace it. The open is non-blocking, so a named pipe
// cannot block the host.
Expected<UniqueFd, std::string> openReadable(const Host::FileSystem &FS,
                                             std::string_view Path) noexcept {
  const auto File = FS.open(Path, static_cast<__wasi_oflags_t>(0),
                            __WASI_RIGHTS_FD_READ, __WASI_FDFLAGS_NONBLOCK);
  if (!File && File.error() == __WASI_ERRNO_NOTCAPABLE) {
    return Unexpected<std::string>(
        "no preopened directory of the guest holds it, or the guest may not "
        "read there"sv);
  }
  if (!File) {
    return Unexpected<std::string>(fmt::format(
        "the caller's filesystem does not open it: {}"sv, File.error()));
  }
  const int Native = *File ? posixFd((*File)->getNativeHandle()) : -1;
  if (Native < 0) {
    return Unexpected<std::string>("the file has no host descriptor"sv);
  }
  UniqueFd Own(::fcntl(Native, F_DUPFD_CLOEXEC, STDERR_FILENO + 1));
  if (Own.get() < 0) {
    return Unexpected<std::string>(
        "the host descriptor cannot be duplicated"sv);
  }
  return Own;
}

#if defined(__linux__)
// Returns the target of the /proc/self/fd link. Refuses a target that fills the
// buffer. The kernel appends " (deleted)" when the file lost its name. This
// function refuses every target with that ending, because a real name with that
// ending looks the same.
std::optional<std::string> kernelPath(int Fd) noexcept {
  constexpr auto kDeleted = " (deleted)"sv;
  const auto Link = fmt::format("/proc/self/fd/{}"sv, Fd);
  std::array<char, PATH_MAX> Buffer;
  const auto Length = ::readlink(Link.c_str(), Buffer.data(), Buffer.size());
  if (Length <= 0 || static_cast<size_t>(Length) >= Buffer.size()) {
    return std::nullopt;
  }
  const std::string_view Target(Buffer.data(), static_cast<size_t>(Length));
  if (Target.front() != '/' ||
      (Target.size() >= kDeleted.size() &&
       Target.substr(Target.size() - kDeleted.size()) == kDeleted)) {
    return std::nullopt;
  }
  return std::string(Target);
}
#elif defined(__APPLE__)
// Returns the path that fcntl reports for Command. Refuses a path that fills
// the buffer.
std::optional<std::string> pathByFcntl(int Fd, int Command) noexcept {
  std::array<char, MAXPATHLEN> Buffer{};
  if (::fcntl(Fd, Command, Buffer.data()) == -1) {
    return std::nullopt;
  }
  const auto Length = ::strnlen(Buffer.data(), Buffer.size());
  if (Length == 0 || Length + 1 >= Buffer.size() || Buffer[0] != '/') {
    return std::nullopt;
  }
  return std::string(Buffer.data(), Length);
}

// Returns the F_GETPATH path. For a file behind a firmlink (/Users/...,
// /private/...), this is the path through the firmlink.
std::optional<std::string> kernelPath(int Fd) noexcept {
  return pathByFcntl(Fd, F_GETPATH);
}
#endif

// Names a start descriptor for a refusal message. A preopened directory gets
// its guest name, never its host path. Every other descriptor gets its position
// in the record, from 0. In a new environment, the position equals the
// descriptor number, so 0 to 2 are the standard streams.
std::string describe(const WASI::VINode *Node, size_t Position) noexcept {
  if (Node == nullptr || Node->name().empty()) {
    return fmt::format(
        "the descriptor at position {} of those the guest started with"sv,
        Position);
  }
  return fmt::format("the preopen \"{}\""sv, logText(Node->name()));
}

// Logs one line with the guest path, the start descriptor that causes the
// refusal, the reason, and the remedy.
ErrNo refuseChangeable(std::string_view GuestPath, const WASI::VINode *Start,
                       size_t Position, std::string_view Why) noexcept {
  spdlog::error("[WasmEdge-StableDiffusion] guest file \"{}\": the guest may "
                "be able to change it: {} is writable and {}; preopen the "
                "file's directory with :readonly, outside every directory the "
                "guest can write"sv,
                logText(GuestPath), describe(Start, Position), Why);
  return ErrNo::NotReadOnly;
}

// Parses an unsigned decimal number that fits in int.
std::optional<int> decimal(std::string_view Text) noexcept {
  if (Text.empty()) {
    return std::nullopt;
  }
  int Value = 0;
  for (const char C : Text) {
    if (C < '0' || C > '9' ||
        Value > (std::numeric_limits<int>::max() - (C - '0')) / 10) {
      return std::nullopt;
    }
    Value = Value * 10 + (C - '0');
  }
  return Value;
}

// Decodes the escapes of a mountinfo path field. The kernel escapes exactly
// these four bytes. Any other escape is an unknown format.
std::optional<std::string> unescape(std::string_view Field) noexcept {
  std::string Text;
  Text.reserve(Field.size());
  while (!Field.empty()) {
    if (Field.front() != '\\') {
      Text.push_back(Field.front());
      Field.remove_prefix(1);
      continue;
    }
    const auto Code = Field.substr(1, 3);
    if (Code == "040"sv) {
      Text.push_back(' ');
    } else if (Code == "011"sv) {
      Text.push_back('\t');
    } else if (Code == "012"sv) {
      Text.push_back('\n');
    } else if (Code == "134"sv) {
      Text.push_back('\\');
    } else {
      return std::nullopt;
    }
    Field.remove_prefix(4);
  }
  return Text;
}

// Parses one row: fields 1 to 6 (mount id, parent id, major:minor, root, mount
// point, options), optional fields, the separator "-", then the file system
// type and the rest. Single spaces separate the fields.
std::optional<MountRow> parseMountRow(std::string_view Line) noexcept {
  std::array<std::string_view, 6> Head;
  for (auto &Field : Head) {
    const auto Space = Line.find(' ');
    if (Space == std::string_view::npos) {
      return std::nullopt;
    }
    Field = Line.substr(0, Space);
    Line.remove_prefix(Space + 1);
  }
  while (true) {
    const auto Space = Line.find(' ');
    if (Space == std::string_view::npos) {
      return std::nullopt;
    }
    const bool Separator = Line.substr(0, Space) == "-"sv;
    Line.remove_prefix(Space + 1);
    if (Separator) {
      break;
    }
  }
  const auto Id = decimal(Head[0]);
  const auto Colon = Head[2].find(':');
  auto Root = unescape(Head[3]);
  auto Point = unescape(Head[4]);
  auto Type = unescape(Line.substr(0, Line.find(' ')));
  if (!Id || !decimal(Head[1]) || Colon == std::string_view::npos ||
      !decimal(Head[2].substr(0, Colon)) ||
      !decimal(Head[2].substr(Colon + 1)) || !Root || Root->empty() || !Point ||
      Point->empty() || Point->front() != '/' || Head[5].empty() || !Type ||
      Type->empty()) {
    return std::nullopt;
  }
  return MountRow{*Id, std::string(Head[2]), std::move(*Root),
                  std::move(*Point), std::move(*Type)};
}

// Returns the single row of a mount id, or nullptr if there is none or more
// than one.
const MountRow *rowOf(Span<const MountRow> Rows, int Id) noexcept {
  const MountRow *Found = nullptr;
  for (const auto &Row : Rows) {
    if (Row.Id == Id) {
      if (Found != nullptr) {
        return nullptr;
      }
      Found = &Row;
    }
  }
  return Found;
}

// Returns the part of Path below directory Dir: empty if Path is Dir, nullopt
// if Path is not in Dir.
std::optional<std::string_view> below(std::string_view Dir,
                                      std::string_view Path) noexcept {
  if (Path == Dir) {
    return ""sv;
  }
  if (!pathContains(Dir, Path)) {
    return std::nullopt;
  }
  Path.remove_prefix(Dir.size());
  if (Path.front() == '/') {
    Path.remove_prefix(1);
  }
  return Path;
}

std::string joinPath(std::string_view Dir, std::string_view Rest) noexcept {
  std::string Joined(Dir);
  if (!Rest.empty()) {
    if (Joined.empty() || Joined.back() != '/') {
      Joined.push_back('/');
    }
    Joined.append(Rest);
  }
  return Joined;
}

// What the rule needs to know about a writable start directory, apart from
// containment: does it reach the file system of the file, and can a file system
// mount on demand at or below it.
struct DirectoryView {
  bool Reaches;
  bool OnDemand;
};

#if defined(__linux__)
constexpr size_t kMaxMountInfoBytes = size_t(16) << 20;
constexpr size_t kMaxFdInfoBytes = size_t(64) << 10;

// Reads a whole /proc file, which reports no size. Returns nullopt if the read
// fails or the file has more than Limit bytes.
std::optional<std::string> readWhole(const char *Name, size_t Limit) noexcept {
  const UniqueFd Fd(::open(Name, O_RDONLY | O_CLOEXEC));
  if (Fd.get() < 0) {
    return std::nullopt;
  }
  constexpr size_t kChunk = size_t(64) << 10;
  std::string Text;
  while (Text.size() <= Limit) {
    const auto Used = Text.size();
    Text.resize(Used + kChunk);
    ssize_t Got;
    do {
      Got = ::read(Fd.get(), Text.data() + Used, kChunk);
    } while (Got < 0 && errno == EINTR);
    if (Got < 0) {
      return std::nullopt;
    }
    Text.resize(Used + static_cast<size_t>(Got));
    if (Got == 0) {
      return Text;
    }
  }
  return std::nullopt;
}

// Returns the mount id of a descriptor: the mnt_id line of its fdinfo.
std::optional<int> mountIdOf(int Fd) noexcept {
  constexpr auto kKey = "mnt_id:"sv;
  const auto Name = fmt::format("/proc/self/fdinfo/{}"sv, Fd);
  const auto Text = readWhole(Name.c_str(), kMaxFdInfoBytes);
  if (!Text) {
    return std::nullopt;
  }
  std::string_view Rest = *Text;
  while (!Rest.empty()) {
    const auto End = std::min(Rest.find('\n'), Rest.size());
    auto Line = Rest.substr(0, End);
    Rest.remove_prefix(std::min(End + 1, Rest.size()));
    if (Line.substr(0, kKey.size()) != kKey) {
      continue;
    }
    Line.remove_prefix(kKey.size());
    while (!Line.empty() && (Line.front() == '\t' || Line.front() == ' ')) {
      Line.remove_prefix(1);
    }
    return decimal(Line);
  }
  return std::nullopt;
}

// What the mount table shows about the file: every path where it is visible,
// and its file system.
struct FileView {
  std::vector<MountRow> Rows;
  std::vector<std::string> Paths;
  std::string Device;
};

Expected<FileView, std::string_view>
viewFile(const LocatedFile &File) noexcept {
  const auto Text = readWhole("/proc/self/mountinfo", kMaxMountInfoBytes);
  if (!Text) {
    return Unexpected<std::string_view>("the mount table cannot be read"sv);
  }
  auto Rows = parseMountInfo(*Text);
  if (!Rows) {
    return Unexpected<std::string_view>(
        "a row of the mount table does not parse"sv);
  }
  const auto Mount = mountIdOf(File.Fd.get());
  const MountRow *Own = Mount ? rowOf(*Rows, *Mount) : nullptr;
  auto Paths = Own ? aliasPaths(*Rows, *Mount, File.HostPath) : std::nullopt;
  if (!Paths) {
    return Unexpected<std::string_view>(
        "the mount table does not show the file"sv);
  }
  std::string Device = Own->Device;
  return FileView{std::move(*Rows), std::move(*Paths), std::move(Device)};
}

// Describes the directory of Fd (kernel path DirPath) from the rows of its
// mount. Returns nullopt if the mount table does not show that mount.
std::optional<DirectoryView> viewDirectory(const FileView &View, int Fd,
                                           std::string_view DirPath,
                                           const struct stat &) noexcept {
  const auto Mount = mountIdOf(Fd);
  if (!Mount) {
    return std::nullopt;
  }
  const auto Reaches =
      reachesFileSystem(View.Rows, *Mount, DirPath, View.Device);
  const auto OnDemand = mountsOnDemandBelow(View.Rows, *Mount, DirPath);
  if (!Reaches || !OnDemand) {
    return std::nullopt;
  }
  return DirectoryView{*Reaches, *OnDemand};
}
#elif defined(__APPLE__)
// The paths and the file system of the file. A file behind a firmlink
// (/Users/..., /private/...) has two paths: the firmlink path (F_GETPATH) and
// the data volume path under /System/Volumes/Data (F_GETPATH_NOFIRMLINK). The
// kernel reports a directory in one of the two forms. macOS cannot mount a
// directory at a second place, so these are all the paths.
struct FileView {
  std::vector<std::string> Paths;
  uint64_t Device;
};

Expected<FileView, std::string_view>
viewFile(const LocatedFile &File) noexcept {
  auto Raw = pathByFcntl(File.Fd.get(), F_GETPATH_NOFIRMLINK);
  if (!Raw) {
    return Unexpected<std::string_view>(
        "the kernel does not name the file without firmlinks"sv);
  }
  std::vector<std::string> Paths{File.HostPath};
  if (*Raw != File.HostPath) {
    Paths.push_back(std::move(*Raw));
  }
  return FileView{std::move(Paths), File.Device};
}

// A volume is mounted once. A volume below a directory puts a path of the file
// inside that directory, and the containment rule refuses it. So "reach" needs
// only the file system of the directory, by st_dev. The system volume and the
// data volume report the same device.
std::optional<DirectoryView> viewDirectory(const FileView &View, int,
                                           std::string_view,
                                           const struct stat &Stat) noexcept {
  return DirectoryView{static_cast<uint64_t>(Stat.st_dev) == View.Device,
                       false};
}
#endif

} // namespace

UniqueFd::UniqueFd(int Fd) noexcept : Fd(Fd) {}

UniqueFd::UniqueFd(UniqueFd &&Other) noexcept
    : Fd(std::exchange(Other.Fd, -1)) {}

UniqueFd &UniqueFd::operator=(UniqueFd &&Other) noexcept {
  if (this != &Other) {
    if (Fd >= 0) {
      ::close(Fd);
    }
    Fd = std::exchange(Other.Fd, -1);
  }
  return *this;
}

UniqueFd::~UniqueFd() noexcept {
  if (Fd >= 0) {
    ::close(Fd);
  }
}

int UniqueFd::get() const noexcept { return Fd; }

// Only WASI::Environ knows the start descriptors. The plug-in and libwasmedge
// each have their own type information for these classes, so the cast works
// only if the C++ runtime compares type names. libstdc++ does. Apple libc++abi
// does as a fallback and notes it in the system log. Otherwise the cast gives
// null and the plug-in refuses every path.
const WASI::Environ *wasiEnviron(const Runtime::CallingFrame &Frame) noexcept {
  return dynamic_cast<const WASI::Environ *>(Frame.getFileSystem());
}

Expected<LocatedFile, ErrNo>
locateGuestFile(const Host::FileSystem &FS,
                std::string_view GuestPath) noexcept {
  if (GuestPath.find('\0') != std::string_view::npos) {
    return refuse(GuestPath, "the path holds a NUL byte"sv);
  }
  if (!GuestPath.empty() && GuestPath.back() == '/') {
    return refuse(GuestPath, "a path that ends in '/' names no file"sv);
  }
  auto Own = openReadable(FS, GuestPath);
  if (!Own) {
    return refuse(GuestPath, Own.error());
  }
  struct stat File{};
  if (::fstat(Own->get(), &File) != 0 || !S_ISREG(File.st_mode)) {
    return refuse(GuestPath, "it is not a regular file"sv);
  }
  auto HostPath = kernelPath(Own->get());
  if (!HostPath) {
    return refuse(GuestPath, "the kernel does not name it"sv);
  }
  struct stat Named{};
  if (::stat(HostPath->c_str(), &Named) != 0 || Named.st_dev != File.st_dev ||
      Named.st_ino != File.st_ino) {
    return refuse(GuestPath, "its host path names another file"sv);
  }
  return LocatedFile{std::move(*Own),
                     std::move(*HostPath),
                     static_cast<uint64_t>(File.st_dev),
                     static_cast<uint64_t>(File.st_ino),
                     static_cast<uint64_t>(File.st_nlink),
                     changeTimeOf(File)};
}

bool pathContains(std::string_view Dir, std::string_view File) noexcept {
  if (Dir.empty() || File.size() <= Dir.size() ||
      File.substr(0, Dir.size()) != Dir) {
    return false;
  }
  return Dir.back() == '/' || File[Dir.size()] == '/';
}

std::optional<std::vector<MountRow>>
parseMountInfo(std::string_view Text) noexcept {
  std::vector<MountRow> Rows;
  while (!Text.empty()) {
    const auto End = Text.find('\n');
    if (End == std::string_view::npos) {
      return std::nullopt;
    }
    auto Row = parseMountRow(Text.substr(0, End));
    if (!Row) {
      return std::nullopt;
    }
    Rows.push_back(std::move(*Row));
    Text.remove_prefix(End + 1);
  }
  return Rows;
}

std::optional<std::vector<std::string>>
aliasPaths(Span<const MountRow> Rows, int MountId,
           std::string_view Path) noexcept {
  const MountRow *Own = rowOf(Rows, MountId);
  if (Own == nullptr) {
    return std::nullopt;
  }
  const auto Rest = below(Own->Point, Path);
  if (!Rest) {
    return std::nullopt;
  }
  const auto Inner = joinPath(Own->Root, *Rest);
  std::vector<std::string> Paths{std::string(Path)};
  for (const auto &Row : Rows) {
    if (&Row == Own || Row.Device != Own->Device) {
      continue;
    }
    const auto Under = below(Row.Root, Inner);
    if (!Under) {
      continue;
    }
    auto Alias = joinPath(Row.Point, *Under);
    if (std::find(Paths.begin(), Paths.end(), Alias) == Paths.end()) {
      Paths.push_back(std::move(Alias));
    }
  }
  return Paths;
}

std::optional<bool> reachesFileSystem(Span<const MountRow> Rows, int DirMountId,
                                      std::string_view DirPath,
                                      std::string_view Device) noexcept {
  const MountRow *Own = rowOf(Rows, DirMountId);
  if (Own == nullptr) {
    return std::nullopt;
  }
  if (Own->Device == Device) {
    return true;
  }
  for (const auto &Row : Rows) {
    if (Row.Device == Device &&
        (Row.Point == DirPath || pathContains(DirPath, Row.Point))) {
      return true;
    }
  }
  return false;
}

std::optional<bool> mountsOnDemandBelow(Span<const MountRow> Rows,
                                        int DirMountId,
                                        std::string_view DirPath) noexcept {
  const MountRow *Own = rowOf(Rows, DirMountId);
  if (Own == nullptr) {
    return std::nullopt;
  }
  if (Own->Type == "autofs"sv) {
    return true;
  }
  for (const auto &Row : Rows) {
    if (Row.Type == "autofs"sv &&
        (Row.Point == DirPath || pathContains(DirPath, Row.Point))) {
      return true;
    }
  }
  return false;
}

Stamp stampOf(std::chrono::system_clock::time_point Moment) noexcept {
  const auto Second = std::chrono::floor<std::chrono::seconds>(Moment);
  return Stamp{
      static_cast<int64_t>(Second.time_since_epoch().count()),
      static_cast<int32_t>(
          std::chrono::duration_cast<std::chrono::nanoseconds>(Moment - Second)
              .count())};
}

Stamp changeTimeOf(const struct stat &Stat) noexcept {
#if defined(__linux__)
  return Stamp{static_cast<int64_t>(Stat.st_ctim.tv_sec),
               static_cast<int32_t>(Stat.st_ctim.tv_nsec)};
#elif defined(__APPLE__)
  return Stamp{static_cast<int64_t>(Stat.st_ctimespec.tv_sec),
               static_cast<int32_t>(Stat.st_ctimespec.tv_nsec)};
#endif
}

// A writable start is a start descriptor that can change files. This function
// refuses the file if, for any writable start:
//   (a) the start is a directory that contains a path of the file (the kernel
//       path or another place in the mount table);
//   (b) the start is the file itself, as a standard stream can be;
//   (c) the start is a directory that reaches the file system of the file, and
//       the file has more than one name or changed since Since;
//   (d) the start is closed, and the file changed since Since.
// It also refuses the file if it cannot check a condition, or if a file system
// can mount on demand at or below the start.
//
// (c) covers a second name of the file in a writable directory. The name
// count is not enough: a guest can open the second name, remove it, and
// continue to write. Each write updates the change time, which a guest cannot
// set. The rule uses reach, not location, because the second name can be behind
// a mount below the directory.
//
// (d) covers a start that was the file itself. The WASI environment keeps only
// the directories of its record open. Thus a closed start is not a directory,
// and the guest cannot write through it again. A write before the close moved
// the change time. This function reads the change time after it sees the close,
// because the file was located before.
ErrNo checkNeverWritable(const WASI::Environ &Env, const LocatedFile &File,
                         std::string_view GuestPath, Stamp Since) noexcept {
  const auto Starts = Env.getInitialDescriptors();
  std::optional<FileView> View;
  for (size_t Position = 0; Position < Starts.size(); ++Position) {
    const auto &Start = Starts[Position];
    if (!isWritableStart(Start.FsRightsBase, Start.FsRightsInheriting)) {
      continue;
    }
    const auto Kept = Start.Node.lock();
    const WASI::VINode *Node = Kept.get();
    const auto Refuse = [GuestPath, Node, Position](std::string_view Why) {
      return refuseChangeable(GuestPath, Node, Position, Why);
    };
    if (Node == nullptr) {
      struct stat Now;
      if (::fstat(File.Fd.get(), &Now) != 0 ||
          changedSince(changeTimeOf(Now), Since)) {
        return Refuse("is closed, and the file's change time moved after the "
                      "plug-in was started"sv);
      }
      continue;
    }
    const auto Native = hostNode(*Node);
    if (!Native) {
      return Refuse("cannot be examined"sv);
    }
    if (static_cast<uint64_t>(Native->Stat.st_dev) == File.Device &&
        static_cast<uint64_t>(Native->Stat.st_ino) == File.Inode) {
      return Refuse("is the file itself"sv);
    }
    if (!S_ISDIR(Native->Stat.st_mode)) {
      continue;
    }
    if (!View) {
      auto Viewed = viewFile(File);
      if (!Viewed) {
        return Refuse(Viewed.error());
      }
      View.emplace(std::move(*Viewed));
    }
    const auto DirPath = kernelPath(Native->Fd);
    if (!DirPath) {
      return Refuse("the kernel does not name it"sv);
    }
    const auto Dir = viewDirectory(*View, Native->Fd, *DirPath, Native->Stat);
    if (!Dir) {
      return Refuse("the mount table does not show its mount"sv);
    }
    if (Dir->OnDemand) {
      return Refuse(
          "a file system can be mounted on demand at it or below it"sv);
    }
    for (const auto &FilePath : View->Paths) {
      if (pathContains(*DirPath, FilePath)) {
        return Refuse("holds it"sv);
      }
    }
    if (Dir->Reaches && File.Links > 1) {
      return Refuse(
          "reaches its file system, and the file has more than one name"sv);
    }
    if (Dir->Reaches && changedSince(File.Changed, Since)) {
      return Refuse("reaches its file system, and the file's change time "
                    "moved after the plug-in was started"sv);
    }
  }
  return ErrNo::Success;
}

} // namespace detail

Expected<std::string, ErrNo>
resolveGuestFile(const Runtime::CallingFrame &Frame, std::string_view GuestPath,
                 std::chrono::system_clock::time_point CreatedAt) noexcept {
  const auto *Env = detail::wasiEnviron(Frame);
  if (Env == nullptr) {
    return detail::refuse(GuestPath,
                          Frame.getFileSystem() == nullptr
                              ? "the calling module imports no WASI, so it "
                                "has no filesystem"sv
                              : "the caller's filesystem is not a WASI "
                                "environment"sv);
  }
  auto File = detail::locateGuestFile(*Env, GuestPath);
  if (!File) {
    return Unexpected<ErrNo>(File.error());
  }
  if (const auto Code = detail::checkNeverWritable(*Env, *File, GuestPath,
                                                   detail::stampOf(CreatedAt));
      Code != ErrNo::Success) {
    return Unexpected<ErrNo>(Code);
  }
  return std::move(File->HostPath);
}

Expected<SessionFiles, ErrNo>
resolveSessionFiles(const Runtime::CallingFrame &Frame,
                    const SessionConfig &Config,
                    std::chrono::system_clock::time_point CreatedAt) noexcept {
  SessionFiles Files;
  const std::array<std::pair<const std::string *, std::string *>, 9> Slots = {{
      {&Config.ModelPath, &Files.Model},
      {&Config.DiffusionModelPath, &Files.DiffusionModel},
      {&Config.ClipLPath, &Files.ClipL},
      {&Config.ClipGPath, &Files.ClipG},
      {&Config.T5xxlPath, &Files.T5xxl},
      {&Config.VaePath, &Files.Vae},
      {&Config.TaesdPath, &Files.Taesd},
      {&Config.ControlNetPath, &Files.ControlNet},
      {&Config.UpscalerModelPath, &Files.Upscaler},
  }};
  for (const auto &[GuestPath, HostPath] : Slots) {
    if (GuestPath->empty()) {
      continue;
    }
    auto Path = resolveGuestFile(Frame, *GuestPath, CreatedAt);
    if (!Path) {
      return Unexpected<ErrNo>(Path.error());
    }
    *HostPath = std::move(*Path);
  }
  Files.Embeddings.reserve(Config.Embeddings.size());
  for (const auto &Embedding : Config.Embeddings) {
    auto Path = resolveGuestFile(Frame, Embedding.Path, CreatedAt);
    if (!Path) {
      return Unexpected<ErrNo>(Path.error());
    }
    Files.Embeddings.push_back(std::move(*Path));
  }
  return Files;
}

Expected<RequestFiles, ErrNo>
resolveRequestFiles(const Runtime::CallingFrame &Frame,
                    const GenerationRequest &Req,
                    std::chrono::system_clock::time_point CreatedAt) noexcept {
  RequestFiles Files;
  Files.Loras.reserve(Req.Loras.size());
  for (const auto &Lora : Req.Loras) {
    auto Path = resolveGuestFile(Frame, Lora.Path, CreatedAt);
    if (!Path) {
      return Unexpected<ErrNo>(Path.error());
    }
    Files.Loras.push_back(std::move(*Path));
  }
  return Files;
}

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
