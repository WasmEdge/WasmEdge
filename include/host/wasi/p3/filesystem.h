// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/filesystem.h - wasi:filesystem host ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of
/// `wasi:filesystem@0.3.1`: `types` with the `descriptor` resource over the
/// preview-1 virtual nodes, and `preopens`.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/component/env.h"
#include "host/wasi/component/fserror.h"
#include "host/wasi/p3/clocks.h"
#include "host/wasi/vinode.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/hostfunc.h"
#include "runtime/component/resourcetable.h"
#include "runtime/component/wit.h"
#include "runtime/instance/component/component.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

/// `descriptor-type`: a variant whose last case `other` names the type.
struct DescriptorType {
  enum Kind : uint32_t {
    BlockDevice,
    CharacterDevice,
    Directory,
    Fifo,
    SymbolicLink,
    RegularFile,
    Socket,
    Other,
  };
  Kind Case = Other;
  std::optional<std::string> Name;
};

/// `error-code`: the 0.2 cases without `would-block`, plus `other`.
struct FsError {
  FsError() = default;
  FsError(WasiComponent::FsErrorCode C) noexcept : Code(C) {}
  WasiComponent::FsErrorCode Code = WasiComponent::FsErrorCode::Io;
  std::optional<std::string> Other;
};

enum class DescriptorFlags : uint32_t {
  Read = 1U << 0,
  Write = 1U << 1,
  FileIntegritySync = 1U << 2,
  DataIntegritySync = 1U << 3,
  RequestedWriteSync = 1U << 4,
  MutateDirectory = 1U << 5,
};

enum class PathFlags : uint32_t {
  SymlinkFollow = 1U << 0,
};

/// True when Flags carries Bit.
template <typename T> bool has(T Flags, T Bit) noexcept {
  return (static_cast<uint32_t>(Flags) & static_cast<uint32_t>(Bit)) != 0;
}

/// The preview-1 lookup flags of PathFlags.
inline __wasi_lookupflags_t lookupFlagsOf(PathFlags Flags) noexcept {
  return has(Flags, PathFlags::SymlinkFollow)
             ? __WASI_LOOKUPFLAGS_SYMLINK_FOLLOW
             : static_cast<__wasi_lookupflags_t>(0);
}

/// Everything a descriptor may do; the open flags narrow read and write.
inline constexpr __wasi_rights_t AllRights = static_cast<__wasi_rights_t>(
    __WASI_RIGHTS_FD_DATASYNC | __WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK |
    __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS | __WASI_RIGHTS_FD_SYNC |
    __WASI_RIGHTS_FD_TELL | __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_ADVISE |
    __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_PATH_CREATE_DIRECTORY |
    __WASI_RIGHTS_PATH_CREATE_FILE | __WASI_RIGHTS_PATH_LINK_SOURCE |
    __WASI_RIGHTS_PATH_LINK_TARGET | __WASI_RIGHTS_PATH_OPEN |
    __WASI_RIGHTS_FD_READDIR | __WASI_RIGHTS_PATH_READLINK |
    __WASI_RIGHTS_PATH_RENAME_SOURCE | __WASI_RIGHTS_PATH_RENAME_TARGET |
    __WASI_RIGHTS_PATH_FILESTAT_GET | __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE |
    __WASI_RIGHTS_PATH_FILESTAT_SET_TIMES | __WASI_RIGHTS_FD_FILESTAT_GET |
    __WASI_RIGHTS_FD_FILESTAT_SET_SIZE | __WASI_RIGHTS_FD_FILESTAT_SET_TIMES |
    __WASI_RIGHTS_PATH_SYMLINK | __WASI_RIGHTS_PATH_REMOVE_DIRECTORY |
    __WASI_RIGHTS_PATH_UNLINK_FILE | __WASI_RIGHTS_POLL_FD_READWRITE);

enum class OpenFlags : uint32_t {
  Create = 1U << 0,
  Directory = 1U << 1,
  Exclusive = 1U << 2,
  Truncate = 1U << 3,
};

enum class Advice : uint32_t {
  Normal,
  Sequential,
  Random,
  WillNeed,
  DontNeed,
  NoReuse,
};

struct DescriptorStat {
  DescriptorType Type;
  uint64_t LinkCount = 0;
  uint64_t Size = 0;
  std::optional<Instant> DataAccess;
  std::optional<Instant> DataModification;
  std::optional<Instant> StatusChange;
};

/// `new-timestamp`: no-change, now, or a given instant.
struct NewTimestamp {
  enum Kind : uint32_t { NoChange, Now, Timestamp };
  Kind Case = NoChange;
  Instant Value;

  /// The preview-1 timestamp: the value given, else 0.
  __wasi_timestamp_t nanos() const noexcept {
    return Case == Timestamp ? Value.nanos() : 0;
  }
  /// The preview-1 flag bits of this timestamp as the access time, else as
  /// the modification time.
  uint32_t fstFlags(bool IsAccess) const noexcept {
    if (Case == Now) {
      return IsAccess ? __WASI_FSTFLAGS_ATIM_NOW : __WASI_FSTFLAGS_MTIM_NOW;
    }
    if (Case == Timestamp) {
      return IsAccess ? __WASI_FSTFLAGS_ATIM : __WASI_FSTFLAGS_MTIM;
    }
    return 0;
  }
};

struct DirectoryEntry {
  DescriptorType Type;
  std::string Name;
};

struct MetadataHashValue {
  uint64_t Lower = 0;
  uint64_t Upper = 0;

  /// The hash of a file identity, mixing its device and inode numbers.
  static MetadataHashValue of(uint64_t Dev, uint64_t Ino) noexcept {
    auto Mix = [](uint64_t A, uint64_t B) noexcept {
      uint64_t Z = A + 0x9e3779b97f4a7c15ULL + (B << 1);
      Z = (Z ^ (Z >> 30)) * 0xbf58476d1ce4e5b9ULL;
      Z = (Z ^ (Z >> 27)) * 0x94d049bb133111ebULL;
      return Z ^ (Z >> 31);
    };
    return MetadataHashValue{Mix(Dev, Ino), Mix(Ino, Dev)};
  }
};

/// The descriptors behind the `descriptor` resource: the tag of its handles
/// is the virtual node type itself.
using DescriptorTable = Runtime::Component::ResourceTable<WASI::VINode>;
using Descriptor = Runtime::Component::Borrow<WASI::VINode>;

template <typename T> using FsResult = Expected<T, FsError>;
using FsStatus = Expected<Runtime::Component::Unit, FsError>;

} // namespace WasiP3
} // namespace Host

namespace Runtime {
namespace Component {

template <> struct Wit<Host::WasiP3::DescriptorType> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"block-device", std::nullopt},
               {"character-device", std::nullopt},
               {"directory", std::nullopt},
               {"fifo", std::nullopt},
               {"symbolic-link", std::nullopt},
               {"regular-file", std::nullopt},
               {"socket", std::nullopt},
               {"other", Wit<std::optional<std::string>>::type(Mint)}});
  }
  static Host::WasiP3::DescriptorType from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::DescriptorType T;
    T.Case = static_cast<Host::WasiP3::DescriptorType::Kind>(Val.Case);
    if (T.Case == Host::WasiP3::DescriptorType::Other && Val.Payload) {
      T.Name = Wit<std::optional<std::string>>::from(*Val.Payload);
    }
    return T;
  }
  static ComponentValVariant into(Host::WasiP3::DescriptorType &&T) noexcept {
    if (T.Case == Host::WasiP3::DescriptorType::Other) {
      return WitVariant::into(
          T.Case, Wit<std::optional<std::string>>::into(std::move(T.Name)));
    }
    return WitVariant::into(T.Case);
  }
};

template <> struct Wit<Host::WasiP3::FsError> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    WitVariant::Cases Cases;
    for (const char *Label : {"access",
                              "already",
                              "bad-descriptor",
                              "busy",
                              "deadlock",
                              "quota",
                              "exist",
                              "file-too-large",
                              "illegal-byte-sequence",
                              "in-progress",
                              "interrupted",
                              "invalid",
                              "io",
                              "is-directory",
                              "loop",
                              "too-many-links",
                              "message-size",
                              "name-too-long",
                              "no-device",
                              "no-entry",
                              "no-lock",
                              "insufficient-memory",
                              "insufficient-space",
                              "not-directory",
                              "not-empty",
                              "not-recoverable",
                              "unsupported",
                              "no-tty",
                              "no-such-device",
                              "overflow",
                              "not-permitted",
                              "pipe",
                              "read-only",
                              "invalid-seek",
                              "text-file-busy",
                              "cross-device"}) {
      Cases.emplace_back(Label, std::nullopt);
    }
    Cases.emplace_back("other", Wit<std::optional<std::string>>::type(Mint));
    return WitVariant::type(Mint, std::move(Cases));
  }
  /// The 0.3 case index of a shared code: `would-block` has no case.
  static uint32_t caseOf(Host::WasiComponent::FsErrorCode Code) noexcept {
    const auto Idx = static_cast<uint32_t>(Code);
    if (Code == Host::WasiComponent::FsErrorCode::WouldBlock) {
      return 36;
    }
    return Idx > 1 ? Idx - 1 : Idx;
  }
  static Host::WasiP3::FsError from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::FsError E;
    if (Val.Case >= 36) {
      E.Code = Host::WasiComponent::FsErrorCode::Io;
      if (Val.Payload) {
        E.Other = Wit<std::optional<std::string>>::from(*Val.Payload);
      }
      return E;
    }
    E.Code = static_cast<Host::WasiComponent::FsErrorCode>(
        Val.Case == 0 ? 0 : Val.Case + 1);
    return E;
  }
  static ComponentValVariant into(Host::WasiP3::FsError &&E) noexcept {
    const uint32_t Case = caseOf(E.Code);
    if (Case == 36) {
      return WitVariant::into(
          Case, Wit<std::optional<std::string>>::into(std::move(E.Other)));
    }
    return WitVariant::into(Case);
  }
};

template <>
struct Wit<Host::WasiP3::DescriptorFlags>
    : WitFlags<Host::WasiP3::DescriptorFlags,
               Wit<Host::WasiP3::DescriptorFlags>> {
  static constexpr const char *Labels[] = {"read",
                                           "write",
                                           "file-integrity-sync",
                                           "data-integrity-sync",
                                           "requested-write-sync",
                                           "mutate-directory"};
};

template <>
struct Wit<Host::WasiP3::PathFlags>
    : WitFlags<Host::WasiP3::PathFlags, Wit<Host::WasiP3::PathFlags>> {
  static constexpr const char *Labels[] = {"symlink-follow"};
};

template <>
struct Wit<Host::WasiP3::OpenFlags>
    : WitFlags<Host::WasiP3::OpenFlags, Wit<Host::WasiP3::OpenFlags>> {
  static constexpr const char *Labels[] = {"create", "directory", "exclusive",
                                           "truncate"};
};

template <>
struct Wit<Host::WasiP3::Advice>
    : WitEnum<Host::WasiP3::Advice, Wit<Host::WasiP3::Advice>> {
  static constexpr const char *Labels[] = {
      "normal", "sequential", "random", "will-need", "dont-need", "no-reuse"};
};

template <> struct Wit<Host::WasiP3::DescriptorStat> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"type", Wit<Host::WasiP3::DescriptorType>::type(Mint)},
               {"link-count", Wit<uint64_t>::type(Mint)},
               {"size", Wit<uint64_t>::type(Mint)},
               {"data-access-timestamp",
                Wit<std::optional<Host::WasiP3::Instant>>::type(Mint)},
               {"data-modification-timestamp",
                Wit<std::optional<Host::WasiP3::Instant>>::type(Mint)},
               {"status-change-timestamp",
                Wit<std::optional<Host::WasiP3::Instant>>::type(Mint)}});
  }
  static Host::WasiP3::DescriptorStat from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    Host::WasiP3::DescriptorStat S;
    S.Type = Wit<Host::WasiP3::DescriptorType>::from(F[0].second);
    S.LinkCount = Wit<uint64_t>::from(F[1].second);
    S.Size = Wit<uint64_t>::from(F[2].second);
    S.DataAccess = Wit<std::optional<Host::WasiP3::Instant>>::from(F[3].second);
    S.DataModification =
        Wit<std::optional<Host::WasiP3::Instant>>::from(F[4].second);
    S.StatusChange =
        Wit<std::optional<Host::WasiP3::Instant>>::from(F[5].second);
    return S;
  }
  static ComponentValVariant into(Host::WasiP3::DescriptorStat &&S) noexcept {
    return WitRecord::into(
        {{"type", Wit<Host::WasiP3::DescriptorType>::into(std::move(S.Type))},
         {"link-count", Wit<uint64_t>::into(S.LinkCount)},
         {"size", Wit<uint64_t>::into(S.Size)},
         {"data-access-timestamp",
          Wit<std::optional<Host::WasiP3::Instant>>::into(
              std::move(S.DataAccess))},
         {"data-modification-timestamp",
          Wit<std::optional<Host::WasiP3::Instant>>::into(
              std::move(S.DataModification))},
         {"status-change-timestamp",
          Wit<std::optional<Host::WasiP3::Instant>>::into(
              std::move(S.StatusChange))}});
  }
};

template <> struct Wit<Host::WasiP3::NewTimestamp> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"no-change", std::nullopt},
               {"now", std::nullopt},
               {"timestamp", Wit<Host::WasiP3::Instant>::type(Mint)}});
  }
  static Host::WasiP3::NewTimestamp from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP3::NewTimestamp T;
    T.Case = static_cast<Host::WasiP3::NewTimestamp::Kind>(Val.Case);
    if (T.Case == Host::WasiP3::NewTimestamp::Timestamp && Val.Payload) {
      T.Value = Wit<Host::WasiP3::Instant>::from(*Val.Payload);
    }
    return T;
  }
  static ComponentValVariant into(Host::WasiP3::NewTimestamp &&T) noexcept {
    if (T.Case == Host::WasiP3::NewTimestamp::Timestamp) {
      return WitVariant::into(
          T.Case, Wit<Host::WasiP3::Instant>::into(std::move(T.Value)));
    }
    return WitVariant::into(T.Case);
  }
};

template <> struct Wit<Host::WasiP3::DirectoryEntry> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"type", Wit<Host::WasiP3::DescriptorType>::type(Mint)},
               {"name", Wit<std::string>::type(Mint)}});
  }
  static Host::WasiP3::DirectoryEntry from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<Host::WasiP3::DescriptorType>::from(F[0].second),
            Wit<std::string>::from(F[1].second)};
  }
  static ComponentValVariant into(Host::WasiP3::DirectoryEntry &&E) noexcept {
    return WitRecord::into(
        {{"type", Wit<Host::WasiP3::DescriptorType>::into(std::move(E.Type))},
         {"name", Wit<std::string>::into(std::move(E.Name))}});
  }
};

template <> struct Wit<Host::WasiP3::MetadataHashValue> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(Mint, {{"lower", Wit<uint64_t>::type(Mint)},
                                  {"upper", Wit<uint64_t>::type(Mint)}});
  }
  static Host::WasiP3::MetadataHashValue from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<uint64_t>::from(F[0].second), Wit<uint64_t>::from(F[1].second)};
  }
  static ComponentValVariant
  into(Host::WasiP3::MetadataHashValue &&H) noexcept {
    return WitRecord::into({{"lower", Wit<uint64_t>::into(H.Lower)},
                            {"upper", Wit<uint64_t>::into(H.Upper)}});
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP3 {

/// The base of the `descriptor` methods: the table their `self` resolves in.
template <typename T>
class DescriptorMethod : public Runtime::Component::HostFunction<T> {
public:
  DescriptorMethod(DescriptorTable &Tab, bool Async = true) noexcept
      : Runtime::Component::HostFunction<T>(Async), Table(Tab) {}

protected:
  /// The node of Self, or the bad-descriptor error.
  WASI::WasiExpect<std::shared_ptr<WASI::VINode>>
  node(Descriptor Self) const noexcept {
    auto Node = Table.get(Self.Rep);
    if (!Node) {
      return WASI::WasiUnexpect(__WASI_ERRNO_BADF);
    }
    return Node;
  }
  /// Run Op on the node of Self as a status result.
  template <typename Op>
  FsStatus status(Descriptor Self, Op &&Run) const noexcept {
    auto Node = node(Self);
    if (!Node) {
      return Unexpected<FsError>(
          FsError(WasiComponent::fsErrorOf(Node.error())));
    }
    if (auto Res = Run(**Node); !Res) {
      return Unexpected<FsError>(
          FsError(WasiComponent::fsErrorOf(Res.error())));
    }
    return Runtime::Component::Unit{};
  }
  DescriptorTable &Table;
};

/// `[method]descriptor.read-via-stream: func(offset: filesize) ->
/// tuple<stream<u8>, future<result<_, error-code>>>`
class DescriptorReadViaStream
    : public DescriptorMethod<DescriptorReadViaStream> {
public:
  static constexpr const char *ParamNames[] = {"self", "offset"};
  DescriptorReadViaStream(DescriptorTable &Tab) noexcept
      : DescriptorMethod(Tab, false) {}
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<std::tuple<Runtime::Component::Stream<uint8_t>,
                    Runtime::Component::Future<FsStatus>>>
  body(Runtime::Component::CallingFrame &Frame, Descriptor Self,
       uint64_t Offset);

private:
  ComponentValType ResultType;
};

/// `[method]descriptor.write-via-stream: func(data: stream<u8>, offset:
/// filesize) -> future<result<_, error-code>>`
class DescriptorWriteViaStream
    : public DescriptorMethod<DescriptorWriteViaStream> {
public:
  static constexpr const char *ParamNames[] = {"self", "data", "offset"};
  DescriptorWriteViaStream(DescriptorTable &Tab) noexcept
      : DescriptorMethod(Tab, false) {}
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<Runtime::Component::Future<FsStatus>>
  body(Runtime::Component::CallingFrame &Frame, Descriptor Self,
       Runtime::Component::Stream<uint8_t> Data, uint64_t Offset);

private:
  ComponentValType ResultType;
};

/// `[method]descriptor.append-via-stream: func(data: stream<u8>) ->
/// future<result<_, error-code>>`
class DescriptorAppendViaStream
    : public DescriptorMethod<DescriptorAppendViaStream> {
public:
  static constexpr const char *ParamNames[] = {"self", "data"};
  DescriptorAppendViaStream(DescriptorTable &Tab) noexcept
      : DescriptorMethod(Tab, false) {}
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<Runtime::Component::Future<FsStatus>>
  body(Runtime::Component::CallingFrame &Frame, Descriptor Self,
       Runtime::Component::Stream<uint8_t> Data);

private:
  ComponentValType ResultType;
};

/// `[method]descriptor.advise: async func(offset: filesize, length: filesize,
/// advice: advice) -> result<_, error-code>`
class DescriptorAdvise : public DescriptorMethod<DescriptorAdvise> {
public:
  static constexpr const char *ParamNames[] = {"self", "offset", "length",
                                               "advice"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        uint64_t Offset, uint64_t Length, Advice Adv);
};

/// `[method]descriptor.sync-data: async func() -> result<_, error-code>`
class DescriptorSyncData : public DescriptorMethod<DescriptorSyncData> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self);
};

/// `[method]descriptor.get-flags: async func() -> result<descriptor-flags,
/// error-code>`
class DescriptorGetFlags : public DescriptorMethod<DescriptorGetFlags> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorFlags>> body(Runtime::Component::CallingFrame &,
                                         Descriptor Self);
};

/// `[method]descriptor.get-type: async func() -> result<descriptor-type,
/// error-code>`
class DescriptorGetType : public DescriptorMethod<DescriptorGetType> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorType>> body(Runtime::Component::CallingFrame &,
                                        Descriptor Self);
};

/// `[method]descriptor.set-size: async func(size: filesize) -> result<_,
/// error-code>`
class DescriptorSetSize : public DescriptorMethod<DescriptorSetSize> {
public:
  static constexpr const char *ParamNames[] = {"self", "size"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        uint64_t Size);
};

/// `[method]descriptor.set-times: async func(data-access-timestamp:
/// new-timestamp, data-modification-timestamp: new-timestamp) -> result<_,
/// error-code>`
class DescriptorSetTimes : public DescriptorMethod<DescriptorSetTimes> {
public:
  static constexpr const char *ParamNames[] = {"self", "data-access-timestamp",
                                               "data-modification-timestamp"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        NewTimestamp Access, NewTimestamp Modification);
};

/// `[method]descriptor.read-directory: func() -> tuple<stream<directory-entry>,
/// future<result<_, error-code>>>`
class DescriptorReadDirectory
    : public DescriptorMethod<DescriptorReadDirectory> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  DescriptorReadDirectory(DescriptorTable &Tab) noexcept
      : DescriptorMethod(Tab, false) {}
  void declare(const Runtime::Component::TypeMinter &Mint) noexcept override;
  Expect<std::tuple<Runtime::Component::Stream<DirectoryEntry>,
                    Runtime::Component::Future<FsStatus>>>
  body(Runtime::Component::CallingFrame &Frame, Descriptor Self);

private:
  ComponentValType EntryType;
  ComponentValType ResultType;
};

/// `[method]descriptor.sync: async func() -> result<_, error-code>`
class DescriptorSync : public DescriptorMethod<DescriptorSync> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self);
};

/// `[method]descriptor.create-directory-at: async func(path: string) ->
/// result<_, error-code>`
class DescriptorCreateDirectoryAt
    : public DescriptorMethod<DescriptorCreateDirectoryAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string Path);
};

/// `[method]descriptor.stat: async func() -> result<descriptor-stat,
/// error-code>`
class DescriptorStatFunc : public DescriptorMethod<DescriptorStatFunc> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorStat>> body(Runtime::Component::CallingFrame &,
                                        Descriptor Self);
};

/// `[method]descriptor.stat-at: async func(path-flags: path-flags, path:
/// string) -> result<descriptor-stat, error-code>`
class DescriptorStatAt : public DescriptorMethod<DescriptorStatAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorStat>> body(Runtime::Component::CallingFrame &,
                                        Descriptor Self, PathFlags Flags,
                                        std::string Path);
};

/// `[method]descriptor.set-times-at: async func(path-flags: path-flags, path:
/// string, data-access-timestamp: new-timestamp, data-modification-timestamp:
/// new-timestamp) -> result<_, error-code>`
class DescriptorSetTimesAt : public DescriptorMethod<DescriptorSetTimesAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path",
                                               "data-access-timestamp",
                                               "data-modification-timestamp"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        PathFlags Flags, std::string Path, NewTimestamp Access,
                        NewTimestamp Modification);
};

/// `[method]descriptor.link-at: async func(old-path-flags: path-flags,
/// old-path: string, new-descriptor: borrow<descriptor>, new-path: string) ->
/// result<_, error-code>`
class DescriptorLinkAt : public DescriptorMethod<DescriptorLinkAt> {
public:
  static constexpr const char *ParamNames[] = {
      "self", "old-path-flags", "old-path", "new-descriptor", "new-path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        PathFlags Flags, std::string OldPath,
                        Descriptor NewDescriptor, std::string NewPath);
};

/// `[method]descriptor.open-at: async func(path-flags: path-flags, path:
/// string, open-flags: open-flags, flags: descriptor-flags) ->
/// result<descriptor, error-code>`
class DescriptorOpenAt : public DescriptorMethod<DescriptorOpenAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path",
                                               "open-flags", "flags"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<Runtime::Component::Own<WASI::VINode>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self, PathFlags Flags,
       std::string Path, OpenFlags Open, DescriptorFlags DFlags);
};

/// `[method]descriptor.readlink-at: async func(path: string) ->
/// result<string, error-code>`
class DescriptorReadlinkAt : public DescriptorMethod<DescriptorReadlinkAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<std::string>> body(Runtime::Component::CallingFrame &,
                                     Descriptor Self, std::string Path);
};

/// `[method]descriptor.remove-directory-at: async func(path: string) ->
/// result<_, error-code>`
class DescriptorRemoveDirectoryAt
    : public DescriptorMethod<DescriptorRemoveDirectoryAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string Path);
};

/// `[method]descriptor.rename-at: async func(old-path: string,
/// new-descriptor: borrow<descriptor>, new-path: string) -> result<_,
/// error-code>`
class DescriptorRenameAt : public DescriptorMethod<DescriptorRenameAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "old-path",
                                               "new-descriptor", "new-path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string OldPath, Descriptor NewDescriptor,
                        std::string NewPath);
};

/// `[method]descriptor.symlink-at: async func(old-path: string, new-path:
/// string) -> result<_, error-code>`
class DescriptorSymlinkAt : public DescriptorMethod<DescriptorSymlinkAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "old-path", "new-path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string OldPath, std::string NewPath);
};

/// `[method]descriptor.unlink-file-at: async func(path: string) -> result<_,
/// error-code>`
class DescriptorUnlinkFileAt : public DescriptorMethod<DescriptorUnlinkFileAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string Path);
};

/// `[method]descriptor.is-same-object: async func(other: borrow<descriptor>)
/// -> bool`
class DescriptorIsSameObject : public DescriptorMethod<DescriptorIsSameObject> {
public:
  static constexpr const char *ParamNames[] = {"self", "other"};
  using DescriptorMethod::DescriptorMethod;
  Expect<bool> body(Runtime::Component::CallingFrame &, Descriptor Self,
                    Descriptor Other);
};

/// `[method]descriptor.metadata-hash: async func() ->
/// result<metadata-hash-value, error-code>`
class DescriptorMetadataHash : public DescriptorMethod<DescriptorMetadataHash> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<MetadataHashValue>> body(Runtime::Component::CallingFrame &,
                                           Descriptor Self);
};

/// `[method]descriptor.metadata-hash-at: async func(path-flags: path-flags,
/// path: string) -> result<metadata-hash-value, error-code>`
class DescriptorMetadataHashAt
    : public DescriptorMethod<DescriptorMetadataHashAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<MetadataHashValue>> body(Runtime::Component::CallingFrame &,
                                           Descriptor Self, PathFlags Flags,
                                           std::string Path);
};

/// `preopens.get-directories: func() -> list<tuple<descriptor, string>>`
class GetDirectories : public Runtime::Component::HostFunction<GetDirectories> {
public:
  GetDirectories(WasiComponent::Env &E, DescriptorTable &Tab) noexcept
      : Env(E), Table(Tab) {}
  Expect<std::vector<
      std::tuple<Runtime::Component::Own<WASI::VINode>, std::string>>>
  body(Runtime::Component::CallingFrame &);

private:
  WasiComponent::Env &Env;
  DescriptorTable &Table;
};

/// `wasi:filesystem/types@0.3.1`: the `descriptor` resource and its methods,
/// plus the named types. It owns the descriptor table.
class FilesystemTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  FilesystemTypesInstance();
  DescriptorTable &descriptors() noexcept { return Descriptors; }

private:
  DescriptorTable Descriptors;
};

/// `wasi:filesystem/preopens@0.3.1`, over the descriptor resource of `types`.
class PreopensInstance : public Runtime::Instance::ComponentInstance {
public:
  PreopensInstance(WasiComponent::Env &Env, FilesystemTypesInstance &Types);
};

/// The two instances of `wasi:filesystem@0.3.1`, `types` first.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeFilesystemInstances(WasiComponent::Env &Env);

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
