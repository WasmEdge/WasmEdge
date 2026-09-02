// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "common/component_valtype.h"
#include "common/component_variant.h"
#include "host/wasi/p3/filesystem.h"
#include "runtime/component/hosttransmit.h"

#include <array>
#include <cstring>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

namespace {

using WasiComponent::FsErrorCode;

FsError errorOf(__wasi_errno_t Errno) noexcept {
  return FsError(WasiComponent::fsErrorOf(Errno));
}

DescriptorType typeOf(__wasi_filetype_t Type) noexcept {
  DescriptorType T;
  switch (Type) {
  case __WASI_FILETYPE_BLOCK_DEVICE:
    T.Case = DescriptorType::BlockDevice;
    break;
  case __WASI_FILETYPE_CHARACTER_DEVICE:
    T.Case = DescriptorType::CharacterDevice;
    break;
  case __WASI_FILETYPE_DIRECTORY:
    T.Case = DescriptorType::Directory;
    break;
  case __WASI_FILETYPE_REGULAR_FILE:
    T.Case = DescriptorType::RegularFile;
    break;
  case __WASI_FILETYPE_SOCKET_DGRAM:
  case __WASI_FILETYPE_SOCKET_STREAM:
    T.Case = DescriptorType::Socket;
    break;
  case __WASI_FILETYPE_SYMBOLIC_LINK:
    T.Case = DescriptorType::SymbolicLink;
    break;
  default:
    T.Case = DescriptorType::Other;
    break;
  }
  return T;
}

DescriptorStat statOf(const __wasi_filestat_t &S) noexcept {
  DescriptorStat Out;
  Out.Type = typeOf(S.filetype);
  Out.LinkCount = S.nlink;
  Out.Size = S.size;
  Out.DataAccess = Instant::fromNanos(S.atim);
  Out.DataModification = Instant::fromNanos(S.mtim);
  Out.StatusChange = Instant::fromNanos(S.ctim);
  return Out;
}

WASI::WasiExpect<__wasi_filestat_t> statOfNode(WASI::VINode &Node) noexcept {
  __wasi_filestat_t S;
  std::memset(&S, 0, sizeof(S));
  EXPECTED_TRY(Node.fdFilestatGet(S));
  return S;
}

WASI::WasiExpect<__wasi_filestat_t>
statOfPath(std::shared_ptr<WASI::VINode> Node, PathFlags Flags,
           std::string_view Path) noexcept {
  __wasi_filestat_t S;
  std::memset(&S, 0, sizeof(S));
  EXPECTED_TRY(WASI::VINode::pathFilestatGet(std::move(Node), Path,
                                             lookupFlagsOf(Flags), S));
  return S;
}

// Write all of Data to Node at Offset.
WASI::WasiExpect<void> writeAllAt(WASI::VINode &Node, Span<const uint8_t> Data,
                                  uint64_t Offset) noexcept {
  while (!Data.empty()) {
    std::array<Span<const uint8_t>, 1> IOVs{Data};
    __wasi_size_t Written = 0;
    EXPECTED_TRY(Node.fdPwrite(IOVs, Offset, Written));
    if (Written == 0) {
      return WASI::WasiUnexpect(__WASI_ERRNO_IO);
    }
    Data = Data.subspan(Written);
    Offset += Written;
  }
  return {};
}

// Resolve the status future of a stream once the stream ends.
void resolveLater(
    const std::shared_ptr<Runtime::Component::HostTransmitEnd> &Done,
    FsStatus &&Status) noexcept {
  Done->writeDetached(
      Runtime::Component::Wit<FsStatus>::into(std::move(Status)));
}

} // namespace

void DescriptorReadViaStream::declare(
    const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<FsStatus>::type(Mint);
}

Expect<std::tuple<Runtime::Component::Stream<uint8_t>,
                  Runtime::Component::Future<FsStatus>>>
DescriptorReadViaStream::body(Runtime::Component::CallingFrame &Frame,
                              Descriptor Self, uint64_t Offset) {
  auto Data = Runtime::Component::HostTransmitEnd::newWritable(
      Frame, true, ComponentValType(ComponentTypeCode::U8));
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  std::tuple<Runtime::Component::Stream<uint8_t>,
             Runtime::Component::Future<FsStatus>>
      Ends{Runtime::Component::Stream<uint8_t>{Data->getGuestValue()},
           Runtime::Component::Future<FsStatus>{Done->getGuestValue()}};
  auto Node = Table.get(Self.Rep);
  if (!Node) {
    Data->drop();
    resolveLater(Done,
                 Unexpected<FsError>(FsError(FsErrorCode::BadDescriptor)));
    return Ends;
  }
  Frame.spawn([Node, Data, Done,
               Offset](Runtime::Component::CallingFrame &F) -> Expect<void> {
    std::vector<uint8_t> Buffer(65536);
    uint64_t Pos = Offset;
    FsStatus Status{Runtime::Component::Unit{}};
    while (true) {
      std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(Buffer)};
      __wasi_size_t Count = 0;
      if (auto Res = Node->fdPread(IOVs, Pos, Count); !Res) {
        Status = Unexpected<FsError>(errorOf(Res.error()));
        break;
      }
      if (Count == 0) {
        break;
      }
      EXPECTED_TRY(auto Outcome,
                   Data->write(F, Span<const uint8_t>(Buffer.data(), Count)));
      if (Outcome.Result !=
          Runtime::Instance::Component::TransmitResult::Completed) {
        break;
      }
      Pos += Count;
    }
    Data->drop();
    resolveLater(Done, std::move(Status));
    return {};
  });
  return Ends;
}

void DescriptorWriteViaStream::declare(
    const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<FsStatus>::type(Mint);
}

Expect<Runtime::Component::Future<FsStatus>> DescriptorWriteViaStream::body(
    Runtime::Component::CallingFrame &Frame, Descriptor Self,
    Runtime::Component::Stream<uint8_t> Data, uint64_t Offset) {
  auto In =
      Runtime::Component::HostTransmitEnd::adoptReadable(Frame, Data.Shared);
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  Runtime::Component::Future<FsStatus> Fut{Done->getGuestValue()};
  auto Node = Table.get(Self.Rep);
  auto Failure = std::make_shared<FsError>(FsErrorCode::BadDescriptor);
  auto Pos = std::make_shared<uint64_t>(Offset);
  In->sink(
      [Node, Failure, Pos](Span<const uint8_t> Bytes) {
        if (!Node) {
          return false;
        }
        if (auto Res = writeAllAt(*Node, Bytes, *Pos); !Res) {
          *Failure = errorOf(Res.error());
          return false;
        }
        *Pos += Bytes.size();
        return true;
      },
      [Done, Failure](bool Failed) {
        resolveLater(Done, Failed ? FsStatus(Unexpected<FsError>(*Failure))
                                  : FsStatus(Runtime::Component::Unit{}));
      });
  return Fut;
}

void DescriptorAppendViaStream::declare(
    const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  ResultType = Runtime::Component::Wit<FsStatus>::type(Mint);
}

Expect<Runtime::Component::Future<FsStatus>>
DescriptorAppendViaStream::body(Runtime::Component::CallingFrame &Frame,
                                Descriptor Self,
                                Runtime::Component::Stream<uint8_t> Data) {
  auto In =
      Runtime::Component::HostTransmitEnd::adoptReadable(Frame, Data.Shared);
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  Runtime::Component::Future<FsStatus> Fut{Done->getGuestValue()};
  auto Node = Table.get(Self.Rep);
  auto Failure = std::make_shared<FsError>(FsErrorCode::BadDescriptor);
  In->sink(
      [Node, Failure](Span<const uint8_t> Bytes) {
        if (!Node) {
          return false;
        }
        __wasi_filesize_t End = 0;
        auto Res = Node->fdSeek(0, __WASI_WHENCE_END, End);
        if (Res) {
          Res = writeAllAt(*Node, Bytes, End);
        }
        if (!Res) {
          *Failure = errorOf(Res.error());
          return false;
        }
        return true;
      },
      [Done, Failure](bool Failed) {
        resolveLater(Done, Failed ? FsStatus(Unexpected<FsError>(*Failure))
                                  : FsStatus(Runtime::Component::Unit{}));
      });
  return Fut;
}

Expect<FsStatus> DescriptorAdvise::body(Runtime::Component::CallingFrame &,
                                        Descriptor Self, uint64_t Offset,
                                        uint64_t Length, Advice Adv) {
  return status(Self, [&](WASI::VINode &Node) {
    return Node.fdAdvise(Offset, Length, static_cast<__wasi_advice_t>(Adv));
  });
}

Expect<FsStatus> DescriptorSyncData::body(Runtime::Component::CallingFrame &,
                                          Descriptor Self) {
  return status(Self, [](WASI::VINode &Node) { return Node.fdDatasync(); });
}

Expect<FsResult<DescriptorFlags>>
DescriptorGetFlags::body(Runtime::Component::CallingFrame &, Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorFlags>(
        Unexpected<FsError>(errorOf(Node.error())));
  }
  __wasi_fdstat_t Stat;
  std::memset(&Stat, 0, sizeof(Stat));
  if (auto Res = (*Node)->fdFdstatGet(Stat); !Res) {
    return FsResult<DescriptorFlags>(Unexpected<FsError>(errorOf(Res.error())));
  }
  uint32_t Flags = 0;
  if ((*Node)->can(__WASI_RIGHTS_FD_READ)) {
    Flags |= static_cast<uint32_t>(DescriptorFlags::Read);
  }
  if ((*Node)->can(__WASI_RIGHTS_FD_WRITE)) {
    Flags |= static_cast<uint32_t>(DescriptorFlags::Write);
  }
  if ((*Node)->isDirectory() && (*Node)->can(__WASI_RIGHTS_PATH_CREATE_FILE)) {
    Flags |= static_cast<uint32_t>(DescriptorFlags::MutateDirectory);
  }
  if (Stat.fs_flags & __WASI_FDFLAGS_SYNC) {
    Flags |= static_cast<uint32_t>(DescriptorFlags::FileIntegritySync);
  }
  if (Stat.fs_flags & __WASI_FDFLAGS_DSYNC) {
    Flags |= static_cast<uint32_t>(DescriptorFlags::DataIntegritySync);
  }
  if (Stat.fs_flags & __WASI_FDFLAGS_RSYNC) {
    Flags |= static_cast<uint32_t>(DescriptorFlags::RequestedWriteSync);
  }
  return FsResult<DescriptorFlags>(static_cast<DescriptorFlags>(Flags));
}

Expect<FsResult<DescriptorType>>
DescriptorGetType::body(Runtime::Component::CallingFrame &, Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorType>(Unexpected<FsError>(errorOf(Node.error())));
  }
  auto Stat = statOfNode(**Node);
  if (!Stat) {
    return FsResult<DescriptorType>(Unexpected<FsError>(errorOf(Stat.error())));
  }
  return FsResult<DescriptorType>(typeOf(Stat->filetype));
}

Expect<FsStatus> DescriptorSetSize::body(Runtime::Component::CallingFrame &,
                                         Descriptor Self, uint64_t Size) {
  return status(Self, [Size](WASI::VINode &Node) {
    return Node.fdFilestatSetSize(Size);
  });
}

Expect<FsStatus> DescriptorSetTimes::body(Runtime::Component::CallingFrame &,
                                          Descriptor Self, NewTimestamp Access,
                                          NewTimestamp Modification) {
  return status(Self, [&](WASI::VINode &Node) {
    return Node.fdFilestatSetTimes(
        Access.nanos(), Modification.nanos(),
        static_cast<__wasi_fstflags_t>(Access.fstFlags(true) |
                                       Modification.fstFlags(false)));
  });
}

void DescriptorReadDirectory::declare(
    const Runtime::Component::TypeMinter &Mint) noexcept {
  HostFunction::declare(Mint);
  EntryType = Runtime::Component::Wit<DirectoryEntry>::type(Mint);
  ResultType = Runtime::Component::Wit<FsStatus>::type(Mint);
}

Expect<std::tuple<Runtime::Component::Stream<DirectoryEntry>,
                  Runtime::Component::Future<FsStatus>>>
DescriptorReadDirectory::body(Runtime::Component::CallingFrame &Frame,
                              Descriptor Self) {
  auto Data =
      Runtime::Component::HostTransmitEnd::newWritable(Frame, true, EntryType);
  auto Done = Runtime::Component::HostTransmitEnd::newWritable(Frame, false,
                                                               ResultType);
  std::tuple<Runtime::Component::Stream<DirectoryEntry>,
             Runtime::Component::Future<FsStatus>>
      Ends{Runtime::Component::Stream<DirectoryEntry>{Data->getGuestValue()},
           Runtime::Component::Future<FsStatus>{Done->getGuestValue()}};
  auto Node = Table.get(Self.Rep);
  if (!Node) {
    Data->drop();
    resolveLater(Done,
                 Unexpected<FsError>(FsError(FsErrorCode::BadDescriptor)));
    return Ends;
  }
  Frame.spawn([Node, Data,
               Done](Runtime::Component::CallingFrame &F) -> Expect<void> {
    std::vector<uint8_t> Buffer(4096);
    __wasi_dircookie_t Cookie = 0;
    FsStatus Status{Runtime::Component::Unit{}};
    bool More = true;
    while (More) {
      __wasi_size_t Size = 0;
      if (auto Res = Node->fdReaddir(Buffer, Cookie, Size); !Res) {
        Status = Unexpected<FsError>(errorOf(Res.error()));
        break;
      }
      // Entries are a dirent followed by the name; the last may be cut off.
      std::vector<ComponentValVariant> Entries;
      size_t Off = 0;
      bool Partial = false;
      while (Off + sizeof(__wasi_dirent_t) <= Size) {
        __wasi_dirent_t Dirent;
        std::memcpy(&Dirent, Buffer.data() + Off, sizeof(Dirent));
        if (Off + sizeof(Dirent) + Dirent.d_namlen > Size) {
          Partial = true;
          break;
        }
        std::string Name(reinterpret_cast<const char *>(Buffer.data() + Off +
                                                        sizeof(Dirent)),
                         Dirent.d_namlen);
        Cookie = Dirent.d_next;
        Off += sizeof(Dirent) + Dirent.d_namlen;
        if (Name != "." && Name != "..") {
          Entries.push_back(Runtime::Component::Wit<DirectoryEntry>::into(
              DirectoryEntry{typeOf(Dirent.d_type), std::move(Name)}));
        }
      }
      if (Off == 0 && Size > 0) {
        Status = Unexpected<FsError>(FsError(FsErrorCode::NameTooLong));
        break;
      }
      More = Partial || Size == Buffer.size();
      if (!Entries.empty()) {
        EXPECTED_TRY(auto Outcome, Data->write(F, std::move(Entries)));
        if (Outcome.Result !=
            Runtime::Instance::Component::TransmitResult::Completed) {
          break;
        }
      }
    }
    Data->drop();
    resolveLater(Done, std::move(Status));
    return {};
  });
  return Ends;
}

Expect<FsStatus> DescriptorSync::body(Runtime::Component::CallingFrame &,
                                      Descriptor Self) {
  return status(Self, [](WASI::VINode &Node) { return Node.fdSync(); });
}

Expect<FsStatus>
DescriptorCreateDirectoryAt::body(Runtime::Component::CallingFrame &,
                                  Descriptor Self, std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsError>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathCreateDirectory(*Node, Path); !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsResult<DescriptorStat>>
DescriptorStatFunc::body(Runtime::Component::CallingFrame &, Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorStat>(Unexpected<FsError>(errorOf(Node.error())));
  }
  auto Stat = statOfNode(**Node);
  if (!Stat) {
    return FsResult<DescriptorStat>(Unexpected<FsError>(errorOf(Stat.error())));
  }
  return FsResult<DescriptorStat>(statOf(*Stat));
}

Expect<FsResult<DescriptorStat>>
DescriptorStatAt::body(Runtime::Component::CallingFrame &, Descriptor Self,
                       PathFlags Flags, std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorStat>(Unexpected<FsError>(errorOf(Node.error())));
  }
  auto Stat = statOfPath(*Node, Flags, Path);
  if (!Stat) {
    return FsResult<DescriptorStat>(Unexpected<FsError>(errorOf(Stat.error())));
  }
  return FsResult<DescriptorStat>(statOf(*Stat));
}

Expect<FsStatus> DescriptorSetTimesAt::body(Runtime::Component::CallingFrame &,
                                            Descriptor Self, PathFlags Flags,
                                            std::string Path,
                                            NewTimestamp Access,
                                            NewTimestamp Modification) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsError>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathFilestatSetTimes(
          *Node, Path, lookupFlagsOf(Flags), Access.nanos(),
          Modification.nanos(),
          static_cast<__wasi_fstflags_t>(Access.fstFlags(true) |
                                         Modification.fstFlags(false)));
      !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsStatus> DescriptorLinkAt::body(Runtime::Component::CallingFrame &,
                                        Descriptor Self, PathFlags Flags,
                                        std::string OldPath,
                                        Descriptor NewDescriptor,
                                        std::string NewPath) {
  auto Old = node(Self);
  auto New = node(NewDescriptor);
  if (!Old || !New) {
    return FsStatus(Unexpected<FsError>(FsError(FsErrorCode::BadDescriptor)));
  }
  if (auto Res = WASI::VINode::pathLink(*Old, OldPath, *New, NewPath,
                                        lookupFlagsOf(Flags));
      !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsResult<Runtime::Component::Own<WASI::VINode>>>
DescriptorOpenAt::body(Runtime::Component::CallingFrame &, Descriptor Self,
                       PathFlags Flags, std::string Path, OpenFlags Open,
                       DescriptorFlags DFlags) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<Runtime::Component::Own<WASI::VINode>>(
        Unexpected<FsError>(errorOf(Node.error())));
  }
  // The preview-1 open mode follows the rights that imply reading or writing.
  uint64_t Rights = AllRights;
  if (!has(DFlags, DescriptorFlags::Read)) {
    Rights &= ~static_cast<uint64_t>(__WASI_RIGHTS_FD_READ |
                                     __WASI_RIGHTS_FD_READDIR);
  }
  if (!has(DFlags, DescriptorFlags::Write)) {
    Rights &= ~static_cast<uint64_t>(
        __WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_DATASYNC |
        __WASI_RIGHTS_FD_ALLOCATE | __WASI_RIGHTS_FD_FILESTAT_SET_SIZE);
  }
  uint32_t OFlags = 0;
  if (has(Open, OpenFlags::Create)) {
    OFlags |= __WASI_OFLAGS_CREAT;
  }
  if (has(Open, OpenFlags::Directory)) {
    OFlags |= __WASI_OFLAGS_DIRECTORY;
  }
  if (has(Open, OpenFlags::Exclusive)) {
    OFlags |= __WASI_OFLAGS_EXCL;
  }
  if (has(Open, OpenFlags::Truncate)) {
    OFlags |= __WASI_OFLAGS_TRUNC;
  }
  uint32_t FdFlags = 0;
  if (has(DFlags, DescriptorFlags::FileIntegritySync)) {
    FdFlags |= __WASI_FDFLAGS_SYNC;
  }
  if (has(DFlags, DescriptorFlags::DataIntegritySync)) {
    FdFlags |= __WASI_FDFLAGS_DSYNC;
  }
  if (has(DFlags, DescriptorFlags::RequestedWriteSync)) {
    FdFlags |= __WASI_FDFLAGS_RSYNC;
  }
  auto Opened = WASI::VINode::pathOpen(
      *Node, Path, lookupFlagsOf(Flags), static_cast<__wasi_oflags_t>(OFlags),
      static_cast<__wasi_rights_t>(Rights), AllRights,
      static_cast<__wasi_fdflags_t>(FdFlags));
  if (!Opened) {
    return FsResult<Runtime::Component::Own<WASI::VINode>>(
        Unexpected<FsError>(errorOf(Opened.error())));
  }
  return FsResult<Runtime::Component::Own<WASI::VINode>>(
      Runtime::Component::Own<WASI::VINode>{Table.add(std::move(*Opened))});
}

Expect<FsResult<std::string>>
DescriptorReadlinkAt::body(Runtime::Component::CallingFrame &, Descriptor Self,
                           std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<std::string>(Unexpected<FsError>(errorOf(Node.error())));
  }
  std::vector<char> Buffer(4096);
  __wasi_size_t Read = 0;
  if (auto Res = WASI::VINode::pathReadlink(*Node, Path, Buffer, Read); !Res) {
    return FsResult<std::string>(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsResult<std::string>(std::string(Buffer.data(), Read));
}

Expect<FsStatus>
DescriptorRemoveDirectoryAt::body(Runtime::Component::CallingFrame &,
                                  Descriptor Self, std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsError>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathRemoveDirectory(*Node, Path); !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsStatus> DescriptorRenameAt::body(Runtime::Component::CallingFrame &,
                                          Descriptor Self, std::string OldPath,
                                          Descriptor NewDescriptor,
                                          std::string NewPath) {
  auto Old = node(Self);
  auto New = node(NewDescriptor);
  if (!Old || !New) {
    return FsStatus(Unexpected<FsError>(FsError(FsErrorCode::BadDescriptor)));
  }
  if (auto Res = WASI::VINode::pathRename(*Old, OldPath, *New, NewPath); !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsStatus> DescriptorSymlinkAt::body(Runtime::Component::CallingFrame &,
                                           Descriptor Self, std::string OldPath,
                                           std::string NewPath) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsError>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathSymlink(OldPath, *Node, NewPath); !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsStatus>
DescriptorUnlinkFileAt::body(Runtime::Component::CallingFrame &,
                             Descriptor Self, std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsError>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathUnlinkFile(*Node, Path); !Res) {
    return FsStatus(Unexpected<FsError>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<bool> DescriptorIsSameObject::body(Runtime::Component::CallingFrame &,
                                          Descriptor Self, Descriptor Other) {
  auto A = node(Self);
  auto B = node(Other);
  if (!A || !B) {
    return false;
  }
  auto SA = statOfNode(**A);
  auto SB = statOfNode(**B);
  if (!SA || !SB) {
    return false;
  }
  return SA->dev == SB->dev && SA->ino == SB->ino;
}

Expect<FsResult<MetadataHashValue>>
DescriptorMetadataHash::body(Runtime::Component::CallingFrame &,
                             Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<MetadataHashValue>(
        Unexpected<FsError>(errorOf(Node.error())));
  }
  auto Stat = statOfNode(**Node);
  if (!Stat) {
    return FsResult<MetadataHashValue>(
        Unexpected<FsError>(errorOf(Stat.error())));
  }
  return FsResult<MetadataHashValue>(
      MetadataHashValue::of(Stat->dev, Stat->ino));
}

Expect<FsResult<MetadataHashValue>>
DescriptorMetadataHashAt::body(Runtime::Component::CallingFrame &,
                               Descriptor Self, PathFlags Flags,
                               std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<MetadataHashValue>(
        Unexpected<FsError>(errorOf(Node.error())));
  }
  auto Stat = statOfPath(*Node, Flags, Path);
  if (!Stat) {
    return FsResult<MetadataHashValue>(
        Unexpected<FsError>(errorOf(Stat.error())));
  }
  return FsResult<MetadataHashValue>(
      MetadataHashValue::of(Stat->dev, Stat->ino));
}

Expect<
    std::vector<std::tuple<Runtime::Component::Own<WASI::VINode>, std::string>>>
GetDirectories::body(Runtime::Component::CallingFrame &) {
  std::vector<std::tuple<Runtime::Component::Own<WASI::VINode>, std::string>>
      Out;
  // The preview-1 environment opens the preopened directories from fd 3 on.
  for (__wasi_fd_t Fd = 3;; ++Fd) {
    auto Node = Env.preview1().getNodeOrNull(Fd);
    if (!Node) {
      break;
    }
    std::string Name = Node->name();
    Out.emplace_back(
        Runtime::Component::Own<WASI::VINode>{Table.add(std::move(Node))},
        std::move(Name));
  }
  return Out;
}

FilesystemTypesInstance::FilesystemTypesInstance()
    : ComponentInstance("wasi:filesystem/types@0.3.1") {
  exportType("descriptor",
             addHostResourceType<WASI::VINode>(
                 [this](uint64_t Rep) { Descriptors.remove(Rep); }));
  const auto Mint = getTypeMinter();
  exportType("filesize", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType("link-count", Mint.primDefType(PrimValType::U64).getTypeIndex());
  exportType(
      "descriptor-type",
      Runtime::Component::Wit<DescriptorType>::type(Mint).getTypeIndex());
  exportType(
      "descriptor-flags",
      Runtime::Component::Wit<DescriptorFlags>::type(Mint).getTypeIndex());
  exportType("path-flags",
             Runtime::Component::Wit<PathFlags>::type(Mint).getTypeIndex());
  exportType("open-flags",
             Runtime::Component::Wit<OpenFlags>::type(Mint).getTypeIndex());
  exportType(
      "descriptor-stat",
      Runtime::Component::Wit<DescriptorStat>::type(Mint).getTypeIndex());
  exportType("new-timestamp",
             Runtime::Component::Wit<NewTimestamp>::type(Mint).getTypeIndex());
  exportType(
      "directory-entry",
      Runtime::Component::Wit<DirectoryEntry>::type(Mint).getTypeIndex());
  exportType("error-code",
             Runtime::Component::Wit<FsError>::type(Mint).getTypeIndex());
  exportType("advice",
             Runtime::Component::Wit<Advice>::type(Mint).getTypeIndex());
  exportType(
      "metadata-hash-value",
      Runtime::Component::Wit<MetadataHashValue>::type(Mint).getTypeIndex());
  addHostFunc("[method]descriptor.read-via-stream",
              std::make_unique<DescriptorReadViaStream>(Descriptors));
  addHostFunc("[method]descriptor.write-via-stream",
              std::make_unique<DescriptorWriteViaStream>(Descriptors));
  addHostFunc("[method]descriptor.append-via-stream",
              std::make_unique<DescriptorAppendViaStream>(Descriptors));
  addHostFunc("[method]descriptor.advise",
              std::make_unique<DescriptorAdvise>(Descriptors));
  addHostFunc("[method]descriptor.sync-data",
              std::make_unique<DescriptorSyncData>(Descriptors));
  addHostFunc("[method]descriptor.get-flags",
              std::make_unique<DescriptorGetFlags>(Descriptors));
  addHostFunc("[method]descriptor.get-type",
              std::make_unique<DescriptorGetType>(Descriptors));
  addHostFunc("[method]descriptor.set-size",
              std::make_unique<DescriptorSetSize>(Descriptors));
  addHostFunc("[method]descriptor.set-times",
              std::make_unique<DescriptorSetTimes>(Descriptors));
  addHostFunc("[method]descriptor.read-directory",
              std::make_unique<DescriptorReadDirectory>(Descriptors));
  addHostFunc("[method]descriptor.sync",
              std::make_unique<DescriptorSync>(Descriptors));
  addHostFunc("[method]descriptor.create-directory-at",
              std::make_unique<DescriptorCreateDirectoryAt>(Descriptors));
  addHostFunc("[method]descriptor.stat",
              std::make_unique<DescriptorStatFunc>(Descriptors));
  addHostFunc("[method]descriptor.stat-at",
              std::make_unique<DescriptorStatAt>(Descriptors));
  addHostFunc("[method]descriptor.set-times-at",
              std::make_unique<DescriptorSetTimesAt>(Descriptors));
  addHostFunc("[method]descriptor.link-at",
              std::make_unique<DescriptorLinkAt>(Descriptors));
  addHostFunc("[method]descriptor.open-at",
              std::make_unique<DescriptorOpenAt>(Descriptors));
  addHostFunc("[method]descriptor.readlink-at",
              std::make_unique<DescriptorReadlinkAt>(Descriptors));
  addHostFunc("[method]descriptor.remove-directory-at",
              std::make_unique<DescriptorRemoveDirectoryAt>(Descriptors));
  addHostFunc("[method]descriptor.rename-at",
              std::make_unique<DescriptorRenameAt>(Descriptors));
  addHostFunc("[method]descriptor.symlink-at",
              std::make_unique<DescriptorSymlinkAt>(Descriptors));
  addHostFunc("[method]descriptor.unlink-file-at",
              std::make_unique<DescriptorUnlinkFileAt>(Descriptors));
  addHostFunc("[method]descriptor.is-same-object",
              std::make_unique<DescriptorIsSameObject>(Descriptors));
  addHostFunc("[method]descriptor.metadata-hash",
              std::make_unique<DescriptorMetadataHash>(Descriptors));
  addHostFunc("[method]descriptor.metadata-hash-at",
              std::make_unique<DescriptorMetadataHashAt>(Descriptors));
}

PreopensInstance::PreopensInstance(WasiComponent::Env &Env,
                                   FilesystemTypesInstance &Types)
    : ComponentInstance("wasi:filesystem/preopens@0.3.1") {
  exportType("descriptor", addSharedResourceType<WASI::VINode>(
                               Types.findTypeResource("descriptor")));
  addHostFunc("get-directories",
              std::make_unique<GetDirectories>(Env, Types.descriptors()));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeFilesystemInstances(WasiComponent::Env &Env) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  auto Types = std::make_unique<FilesystemTypesInstance>();
  auto Preopens = std::make_unique<PreopensInstance>(Env, *Types);
  Out.push_back(std::move(Types));
  Out.push_back(std::move(Preopens));
  return Out;
}

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
