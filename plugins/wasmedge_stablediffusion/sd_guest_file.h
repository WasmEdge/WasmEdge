// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "sd_abi.h"
#include "sd_backend.h"
#include "sd_request.h"

#include "common/expected.h"
#include "common/span.h"
#include "host/filesystem.h"
#include "host/wasi/environ.h"
#include "runtime/callingframe.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace StableDiffusion {

// Guest paths and the read-only rule.
//
// The library opens its files by host path, more than once. The plug-in gives
// it a host path only for a file that the guest could never change.
// resolveGuestFile:
//   1. opens the guest path with the caller filesystem, inside the preopen and
//      with the rights of the guest;
//   2. takes the host path that the kernel reports for the opened file;
//   3. refuses the file with NotReadOnly if a start descriptor could have
//      changed it (checkNeverWritable);
//   4. keeps no descriptor and makes no copy.
//
// A start descriptor is a descriptor that the WASI environment started with.
// The rule uses these, not the current table: a guest can close a writable
// preopen and keep a directory below it. Every WASI operation of a guest starts
// from a start descriptor and has at most its rights.
//
// The rule cannot see a change from outside the WASI environment. DESIGN.md
// lists what the embedder must prevent.
namespace detail {

class UniqueFd { // closes a POSIX descriptor; move-only
public:
  explicit UniqueFd(int Fd = -1) noexcept;
  UniqueFd(UniqueFd &&Other) noexcept;
  UniqueFd &operator=(UniqueFd &&Other) noexcept;
  UniqueFd(const UniqueFd &) = delete;
  UniqueFd &operator=(const UniqueFd &) = delete;
  ~UniqueFd() noexcept;
  int get() const noexcept;

private:
  int Fd;
};
// Seconds and nanoseconds since the epoch.
struct Stamp {
  int64_t Sec;
  int32_t Nsec;
};
struct LocatedFile {
  UniqueFd Fd;          // the plug-in's own descriptor on the file
  std::string HostPath; // the path the kernel reports for it
  uint64_t Device, Inode, Links;
  Stamp Changed; // its change time
};
// Returns the WASI environment of the caller filesystem. The read-only rule
// needs its record of start descriptors. Returns nullptr if the caller has no
// filesystem or the filesystem is not a WASI environment.
const WASI::Environ *wasiEnviron(const Runtime::CallingFrame &Frame) noexcept;
// Opens a guest path with the caller filesystem, which finds the preopen and
// keeps the lookup inside it. Returns the file with its host path. Returns
// LoadFailed on every failure.
Expected<LocatedFile, ErrNo>
locateGuestFile(const Host::FileSystem &FS,
                std::string_view GuestPath) noexcept;

// The rights that let a descriptor change a file or the names that lead to a
// file.
inline constexpr __wasi_rights_t kChangingRights =
    __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_ALLOCATE |
    __WASI_RIGHTS_FD_FILESTAT_SET_SIZE | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
    __WASI_RIGHTS_PATH_CREATE_DIRECTORY | __WASI_RIGHTS_PATH_CREATE_FILE |
    __WASI_RIGHTS_PATH_LINK_SOURCE | __WASI_RIGHTS_PATH_LINK_TARGET |
    __WASI_RIGHTS_PATH_RENAME_SOURCE | __WASI_RIGHTS_PATH_RENAME_TARGET |
    __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
    __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_PATH_SYMLINK |
    __WASI_RIGHTS_PATH_REMOVE_DIRECTORY | __WASI_RIGHTS_PATH_UNLINK_FILE;
// Returns true when a start descriptor with these recorded rights is a writable
// start.
constexpr bool isWritableStart(__wasi_rights_t Base,
                               __wasi_rights_t Inheriting) noexcept {
  return ((Base | Inheriting) & kChangingRights) !=
         static_cast<__wasi_rights_t>(0);
}
// Returns true when File is in directory Dir or below it. It compares whole
// components only.
bool pathContains(std::string_view Dir, std::string_view File) noexcept;
struct MountRow { // a row of /proc/self/mountinfo
  int Id;
  // Root and Point have their escapes decoded. Type is the file system type.
  std::string Device, Root, Point, Type;
};
// Returns nullopt if any row does not parse.
std::optional<std::vector<MountRow>>
parseMountInfo(std::string_view Text) noexcept;
// Returns every path where the file is visible. The file has the path Path on
// mount MountId. The result holds Path and the place of the file under every
// other row of the same Device whose Root is the file itself (a file bound over
// a file), the directory of the file, or a directory above it.
std::optional<std::vector<std::string>>
aliasPaths(Span<const MountRow> Rows, int MountId,
           std::string_view Path) noexcept;
// A file system can stamp a change earlier than the time it occurred. Linux
// stamps files from a clock that can be one tick behind the clock that gives
// Since. 20 ms covers two ticks.
inline constexpr int32_t kStampSlackNsec = 20'000'000;
// Returns true when a file with change time Changed could have changed at Since
// or later: Changed is not earlier than Since minus the slack. A Changed with
// no fraction is compared by the second, because a file system with
// whole-second stamps truncates the time.
constexpr bool changedSince(Stamp Changed, Stamp Since) noexcept {
  Stamp Threshold = Since;
  if (Threshold.Nsec < kStampSlackNsec) {
    if (Threshold.Sec == std::numeric_limits<int64_t>::min()) {
      return true;
    }
    --Threshold.Sec;
    Threshold.Nsec += 1'000'000'000;
  }
  Threshold.Nsec -= kStampSlackNsec;
  if (Changed.Nsec == 0) {
    return Changed.Sec >= Threshold.Sec;
  }
  return Changed.Sec > Threshold.Sec ||
         (Changed.Sec == Threshold.Sec && Changed.Nsec >= Threshold.Nsec);
}
Stamp stampOf(std::chrono::system_clock::time_point Moment) noexcept;
Stamp changeTimeOf(const struct stat &Stat) noexcept;
// Returns true when the directory at DirPath on mount DirMountId reaches file
// system Device: its own mount is that file system, or a mount of that file
// system is at or below DirPath. Returns nullopt if Rows does not show the
// mount of the directory.
std::optional<bool> reachesFileSystem(Span<const MountRow> Rows, int DirMountId,
                                      std::string_view DirPath,
                                      std::string_view Device) noexcept;
// Returns true when a file system can mount on demand at or below DirPath: the
// mount of the directory is autofs, or an autofs mount point is there. Returns
// nullopt if Rows does not show the mount of the directory.
std::optional<bool> mountsOnDemandBelow(Span<const MountRow> Rows,
                                        int DirMountId,
                                        std::string_view DirPath) noexcept;
// Returns Success when no start descriptor could have changed the file. A
// change counts from Since on. Returns NotReadOnly otherwise, also when the
// check cannot be done. Refusal messages name GuestPath.
ErrNo checkNeverWritable(const WASI::Environ &Env, const LocatedFile &File,
                         std::string_view GuestPath, Stamp Since) noexcept;

} // namespace detail

// Returns the host path of a guest path, for a file that the guest could never
// change. CreatedAt is the creation time of the module instance that serves the
// guest.
Expected<std::string, ErrNo>
resolveGuestFile(const Runtime::CallingFrame &Frame, std::string_view GuestPath,
                 std::chrono::system_clock::time_point CreatedAt) noexcept;
// Returns the host paths of all files that a configuration names, or the first
// failure.
Expected<SessionFiles, ErrNo>
resolveSessionFiles(const Runtime::CallingFrame &Frame,
                    const SessionConfig &Config,
                    std::chrono::system_clock::time_point CreatedAt) noexcept;
// Returns the host paths of all LoRAs that a request names, or the first
// failure.
Expected<RequestFiles, ErrNo>
resolveRequestFiles(const Runtime::CallingFrame &Frame,
                    const GenerationRequest &Req,
                    std::chrono::system_clock::time_point CreatedAt) noexcept;

} // namespace StableDiffusion
} // namespace Host
} // namespace WasmEdge
