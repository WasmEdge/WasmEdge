// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p2/filesystem.h - wasi:filesystem host ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host component instances of
/// `wasi:filesystem@0.2.12`: `types` with the `descriptor` and
/// `directory-entry-stream` resources over the preview-1 virtual nodes, and
/// `preopens`. The flags and the advice enum are those of 0.3.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "common/errcode.h"
#include "common/expected.h"
#include "host/wasi/component/env.h"
#include "host/wasi/component/fserror.h"
#include "host/wasi/p2/clocks.h"
#include "host/wasi/p2/io.h"
#include "host/wasi/p3/filesystem.h"
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
namespace WasiP2 {

/// `descriptor-type` of 0.2: an enum with `unknown`.
enum class DescriptorType : uint32_t {
  Unknown,
  BlockDevice,
  CharacterDevice,
  Directory,
  Fifo,
  SymbolicLink,
  RegularFile,
  Socket,
};

using WasiComponent::FsErrorCode;
using WasiP3::Advice;
using WasiP3::DescriptorFlags;
using WasiP3::MetadataHashValue;
using WasiP3::OpenFlags;
using WasiP3::PathFlags;

struct DescriptorStat {
  DescriptorType Type = DescriptorType::Unknown;
  uint64_t LinkCount = 0;
  uint64_t Size = 0;
  std::optional<Datetime> DataAccess;
  std::optional<Datetime> DataModification;
  std::optional<Datetime> StatusChange;
};

/// `new-timestamp`: no-change, now, or a given datetime.
struct NewTimestamp {
  enum Kind : uint32_t { NoChange, Now, Timestamp };
  Kind Case = NoChange;
  Datetime Value;

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
  DescriptorType Type = DescriptorType::Unknown;
  std::string Name;
};

/// The entries of a directory, read when the stream is opened.
struct DirectoryEntryStream {
  std::vector<DirectoryEntry> Entries;
  size_t Next = 0;
};

using DescriptorTable = Runtime::Component::ResourceTable<WASI::VINode>;
using DirectoryTable = Runtime::Component::ResourceTable<DirectoryEntryStream>;
using Descriptor = Runtime::Component::Borrow<WASI::VINode>;
template <typename T> using FsResult = Expected<T, FsErrorCode>;
using FsStatus = Expected<Runtime::Component::Unit, FsErrorCode>;

} // namespace WasiP2
} // namespace Host

namespace Runtime {
namespace Component {

template <>
struct Wit<Host::WasiP2::DescriptorType>
    : WitEnum<Host::WasiP2::DescriptorType, Wit<Host::WasiP2::DescriptorType>> {
  static constexpr const char *Labels[] = {
      "unknown", "block-device",  "character-device", "directory",
      "fifo",    "symbolic-link", "regular-file",     "socket"};
};

template <>
struct Wit<Host::WasiComponent::FsErrorCode>
    : WitEnum<Host::WasiComponent::FsErrorCode,
              Wit<Host::WasiComponent::FsErrorCode>> {
  static constexpr const char *Labels[] = {"access",
                                           "would-block",
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
                                           "cross-device"};
};

template <> struct Wit<Host::WasiP2::DescriptorStat> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"type", Wit<Host::WasiP2::DescriptorType>::type(Mint)},
               {"link-count", Wit<uint64_t>::type(Mint)},
               {"size", Wit<uint64_t>::type(Mint)},
               {"data-access-timestamp",
                Wit<std::optional<Host::WasiP2::Datetime>>::type(Mint)},
               {"data-modification-timestamp",
                Wit<std::optional<Host::WasiP2::Datetime>>::type(Mint)},
               {"status-change-timestamp",
                Wit<std::optional<Host::WasiP2::Datetime>>::type(Mint)}});
  }
  static Host::WasiP2::DescriptorStat from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    Host::WasiP2::DescriptorStat S;
    S.Type = Wit<Host::WasiP2::DescriptorType>::from(F[0].second);
    S.LinkCount = Wit<uint64_t>::from(F[1].second);
    S.Size = Wit<uint64_t>::from(F[2].second);
    S.DataAccess =
        Wit<std::optional<Host::WasiP2::Datetime>>::from(F[3].second);
    S.DataModification =
        Wit<std::optional<Host::WasiP2::Datetime>>::from(F[4].second);
    S.StatusChange =
        Wit<std::optional<Host::WasiP2::Datetime>>::from(F[5].second);
    return S;
  }
  static ComponentValVariant into(Host::WasiP2::DescriptorStat &&S) noexcept {
    return WitRecord::into(
        {{"type", Wit<Host::WasiP2::DescriptorType>::into(S.Type)},
         {"link-count", Wit<uint64_t>::into(S.LinkCount)},
         {"size", Wit<uint64_t>::into(S.Size)},
         {"data-access-timestamp",
          Wit<std::optional<Host::WasiP2::Datetime>>::into(
              std::move(S.DataAccess))},
         {"data-modification-timestamp",
          Wit<std::optional<Host::WasiP2::Datetime>>::into(
              std::move(S.DataModification))},
         {"status-change-timestamp",
          Wit<std::optional<Host::WasiP2::Datetime>>::into(
              std::move(S.StatusChange))}});
  }
};

template <> struct Wit<Host::WasiP2::NewTimestamp> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitVariant::type(
        Mint, {{"no-change", std::nullopt},
               {"now", std::nullopt},
               {"timestamp", Wit<Host::WasiP2::Datetime>::type(Mint)}});
  }
  static Host::WasiP2::NewTimestamp from(const ComponentValVariant &V) {
    const auto &Val = WitVariant::value(V);
    Host::WasiP2::NewTimestamp T;
    T.Case = static_cast<Host::WasiP2::NewTimestamp::Kind>(Val.Case);
    if (T.Case == Host::WasiP2::NewTimestamp::Timestamp && Val.Payload) {
      T.Value = Wit<Host::WasiP2::Datetime>::from(*Val.Payload);
    }
    return T;
  }
  static ComponentValVariant into(Host::WasiP2::NewTimestamp &&T) noexcept {
    if (T.Case == Host::WasiP2::NewTimestamp::Timestamp) {
      return WitVariant::into(
          T.Case, Wit<Host::WasiP2::Datetime>::into(std::move(T.Value)));
    }
    return WitVariant::into(T.Case);
  }
};

template <> struct Wit<Host::WasiP2::DirectoryEntry> {
  static ComponentValType type(const TypeMinter &Mint) noexcept {
    return WitRecord::type(
        Mint, {{"type", Wit<Host::WasiP2::DescriptorType>::type(Mint)},
               {"name", Wit<std::string>::type(Mint)}});
  }
  static Host::WasiP2::DirectoryEntry from(const ComponentValVariant &V) {
    auto F = WitRecord::fields(V);
    return {Wit<Host::WasiP2::DescriptorType>::from(F[0].second),
            Wit<std::string>::from(F[1].second)};
  }
  static ComponentValVariant into(Host::WasiP2::DirectoryEntry &&E) noexcept {
    return WitRecord::into(
        {{"type", Wit<Host::WasiP2::DescriptorType>::into(E.Type)},
         {"name", Wit<std::string>::into(std::move(E.Name))}});
  }
};

} // namespace Component
} // namespace Runtime

namespace Host {
namespace WasiP2 {

/// The base of the `descriptor` methods: the tables they resolve in.
template <typename T>
class DescriptorMethod : public Runtime::Component::HostFunction<T> {
public:
  DescriptorMethod(IoHost &H, DescriptorTable &Tab) noexcept
      : Host(H), Table(Tab) {}

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
      return Unexpected<FsErrorCode>(WasiComponent::fsErrorOf(Node.error()));
    }
    if (auto Res = Run(**Node); !Res) {
      return Unexpected<FsErrorCode>(WasiComponent::fsErrorOf(Res.error()));
    }
    return Runtime::Component::Unit{};
  }
  IoHost &Host;
  DescriptorTable &Table;
};

/// `[method]descriptor.read-via-stream: func(offset: filesize) ->
/// result<input-stream, error-code>`
class DescriptorReadViaStream
    : public DescriptorMethod<DescriptorReadViaStream> {
public:
  static constexpr const char *ParamNames[] = {"self", "offset"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<Runtime::Component::Own<InputStream>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self, uint64_t Offset);
};

/// `[method]descriptor.write-via-stream: func(offset: filesize) ->
/// result<output-stream, error-code>` and `append-via-stream`
class DescriptorWriteViaStream
    : public DescriptorMethod<DescriptorWriteViaStream> {
public:
  static constexpr const char *ParamNames[] = {"self", "offset"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<Runtime::Component::Own<OutputStream>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self, uint64_t Offset);
};

class DescriptorAppendViaStream
    : public DescriptorMethod<DescriptorAppendViaStream> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<Runtime::Component::Own<OutputStream>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self);
};

/// `[method]descriptor.advise`
class DescriptorAdvise : public DescriptorMethod<DescriptorAdvise> {
public:
  static constexpr const char *ParamNames[] = {"self", "offset", "length",
                                               "advice"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        uint64_t Offset, uint64_t Length, Advice Adv);
};

/// `[method]descriptor.sync-data` and `sync`
class DescriptorSync : public DescriptorMethod<DescriptorSync> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  DescriptorSync(IoHost &H, DescriptorTable &Tab, bool DataOnly) noexcept
      : DescriptorMethod(H, Tab), DataOnly(DataOnly) {}
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self);

private:
  bool DataOnly;
};

/// `[method]descriptor.get-flags`
class DescriptorGetFlags : public DescriptorMethod<DescriptorGetFlags> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorFlags>> body(Runtime::Component::CallingFrame &,
                                         Descriptor Self);
};

/// `[method]descriptor.get-type`
class DescriptorGetType : public DescriptorMethod<DescriptorGetType> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorType>> body(Runtime::Component::CallingFrame &,
                                        Descriptor Self);
};

/// `[method]descriptor.set-size`
class DescriptorSetSize : public DescriptorMethod<DescriptorSetSize> {
public:
  static constexpr const char *ParamNames[] = {"self", "size"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        uint64_t Size);
};

/// `[method]descriptor.set-times`
class DescriptorSetTimes : public DescriptorMethod<DescriptorSetTimes> {
public:
  static constexpr const char *ParamNames[] = {"self", "data-access-timestamp",
                                               "data-modification-timestamp"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        NewTimestamp Access, NewTimestamp Modification);
};

/// `[method]descriptor.read: func(length: filesize, offset: filesize) ->
/// result<tuple<list<u8>, bool>, error-code>`
class DescriptorRead : public DescriptorMethod<DescriptorRead> {
public:
  static constexpr const char *ParamNames[] = {"self", "length", "offset"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<std::tuple<std::vector<uint8_t>, bool>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self, uint64_t Length,
       uint64_t Offset);
};

/// `[method]descriptor.write: func(buffer: list<u8>, offset: filesize) ->
/// result<filesize, error-code>`
class DescriptorWrite : public DescriptorMethod<DescriptorWrite> {
public:
  static constexpr const char *ParamNames[] = {"self", "buffer", "offset"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<uint64_t>> body(Runtime::Component::CallingFrame &,
                                  Descriptor Self, std::vector<uint8_t> Buffer,
                                  uint64_t Offset);
};

/// `[method]descriptor.read-directory: func() ->
/// result<directory-entry-stream, error-code>`
class DescriptorReadDirectory
    : public DescriptorMethod<DescriptorReadDirectory> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  DescriptorReadDirectory(IoHost &H, DescriptorTable &Tab,
                          DirectoryTable &Dirs) noexcept
      : DescriptorMethod(H, Tab), Directories(Dirs) {}
  Expect<FsResult<Runtime::Component::Own<DirectoryEntryStream>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self);

private:
  DirectoryTable &Directories;
};

/// `[method]descriptor.create-directory-at`, `remove-directory-at`,
/// `unlink-file-at`, and `readlink-at`: one path argument.
class DescriptorPathOp : public DescriptorMethod<DescriptorPathOp> {
public:
  enum class Op { CreateDirectory, RemoveDirectory, UnlinkFile };
  static constexpr const char *ParamNames[] = {"self", "path"};
  DescriptorPathOp(IoHost &H, DescriptorTable &Tab, Op Which) noexcept
      : DescriptorMethod(H, Tab), Which(Which) {}
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string Path);

private:
  Op Which;
};

class DescriptorReadlinkAt : public DescriptorMethod<DescriptorReadlinkAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<std::string>> body(Runtime::Component::CallingFrame &,
                                     Descriptor Self, std::string Path);
};

/// `[method]descriptor.stat` and `stat-at`
class DescriptorStatFunc : public DescriptorMethod<DescriptorStatFunc> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorStat>> body(Runtime::Component::CallingFrame &,
                                        Descriptor Self);
};

class DescriptorStatAt : public DescriptorMethod<DescriptorStatAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<DescriptorStat>> body(Runtime::Component::CallingFrame &,
                                        Descriptor Self, PathFlags Flags,
                                        std::string Path);
};

/// `[method]descriptor.set-times-at`
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

/// `[method]descriptor.link-at`
class DescriptorLinkAt : public DescriptorMethod<DescriptorLinkAt> {
public:
  static constexpr const char *ParamNames[] = {
      "self", "old-path-flags", "old-path", "new-descriptor", "new-path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        PathFlags Flags, std::string OldPath,
                        Descriptor NewDescriptor, std::string NewPath);
};

/// `[method]descriptor.open-at`
class DescriptorOpenAt : public DescriptorMethod<DescriptorOpenAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path",
                                               "open-flags", "flags"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<Runtime::Component::Own<WASI::VINode>>>
  body(Runtime::Component::CallingFrame &, Descriptor Self, PathFlags Flags,
       std::string Path, OpenFlags Open, DescriptorFlags DFlags);
};

/// `[method]descriptor.rename-at`
class DescriptorRenameAt : public DescriptorMethod<DescriptorRenameAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "old-path",
                                               "new-descriptor", "new-path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string OldPath, Descriptor NewDescriptor,
                        std::string NewPath);
};

/// `[method]descriptor.symlink-at`
class DescriptorSymlinkAt : public DescriptorMethod<DescriptorSymlinkAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "old-path", "new-path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsStatus> body(Runtime::Component::CallingFrame &, Descriptor Self,
                        std::string OldPath, std::string NewPath);
};

/// `[method]descriptor.is-same-object`
class DescriptorIsSameObject : public DescriptorMethod<DescriptorIsSameObject> {
public:
  static constexpr const char *ParamNames[] = {"self", "other"};
  using DescriptorMethod::DescriptorMethod;
  Expect<bool> body(Runtime::Component::CallingFrame &, Descriptor Self,
                    Descriptor Other);
};

/// `[method]descriptor.metadata-hash` and `metadata-hash-at`
class DescriptorMetadataHash : public DescriptorMethod<DescriptorMetadataHash> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<MetadataHashValue>> body(Runtime::Component::CallingFrame &,
                                           Descriptor Self);
};

class DescriptorMetadataHashAt
    : public DescriptorMethod<DescriptorMetadataHashAt> {
public:
  static constexpr const char *ParamNames[] = {"self", "path-flags", "path"};
  using DescriptorMethod::DescriptorMethod;
  Expect<FsResult<MetadataHashValue>> body(Runtime::Component::CallingFrame &,
                                           Descriptor Self, PathFlags Flags,
                                           std::string Path);
};

/// `[method]directory-entry-stream.read-directory-entry: func() ->
/// result<option<directory-entry>, error-code>`
class ReadDirectoryEntry
    : public Runtime::Component::HostFunction<ReadDirectoryEntry> {
public:
  static constexpr const char *ParamNames[] = {"self"};
  ReadDirectoryEntry(DirectoryTable &Dirs) noexcept : Directories(Dirs) {}
  Expect<FsResult<std::optional<DirectoryEntry>>>
  body(Runtime::Component::CallingFrame &,
       Runtime::Component::Borrow<DirectoryEntryStream> Self);

private:
  DirectoryTable &Directories;
};

/// `filesystem-error-code: func(err: borrow<error>) -> option<error-code>`
class FilesystemErrorCode
    : public Runtime::Component::HostFunction<FilesystemErrorCode> {
public:
  static constexpr const char *ParamNames[] = {"err"};
  FilesystemErrorCode(IoHost &H) noexcept : Host(H) {}
  Expect<std::optional<FsErrorCode>> body(Runtime::Component::CallingFrame &,
                                          ErrorBorrow Err);

private:
  IoHost &Host;
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

/// `wasi:filesystem/types@0.2.12`: the resources, their methods, and the
/// named types. It owns the descriptor and directory tables.
class FilesystemTypesInstance : public Runtime::Instance::ComponentInstance {
public:
  FilesystemTypesInstance(IoHost &Host);
  DescriptorTable &descriptors() noexcept { return Descriptors; }

private:
  DescriptorTable Descriptors;
  DirectoryTable Directories;
};

/// `wasi:filesystem/preopens@0.2.12`, over the descriptor resource of `types`.
class PreopensInstance : public Runtime::Instance::ComponentInstance {
public:
  PreopensInstance(WasiComponent::Env &Env, FilesystemTypesInstance &Types);
};

/// The two instances of `wasi:filesystem@0.2.12`, `types` first.
std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeFilesystemInstances(WasiComponent::Env &Env, IoHost &Host);

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
