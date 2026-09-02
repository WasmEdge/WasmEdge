// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#include "host/wasi/p2/filesystem.h"

#include <array>
#include <cstring>
#include <memory>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP2 {

namespace {

FsErrorCode errorOf(__wasi_errno_t Errno) noexcept {
  return WasiComponent::fsErrorOf(Errno);
}

DescriptorType typeOf(__wasi_filetype_t Type) noexcept {
  switch (Type) {
  case __WASI_FILETYPE_BLOCK_DEVICE:
    return DescriptorType::BlockDevice;
  case __WASI_FILETYPE_CHARACTER_DEVICE:
    return DescriptorType::CharacterDevice;
  case __WASI_FILETYPE_DIRECTORY:
    return DescriptorType::Directory;
  case __WASI_FILETYPE_REGULAR_FILE:
    return DescriptorType::RegularFile;
  case __WASI_FILETYPE_SOCKET_DGRAM:
  case __WASI_FILETYPE_SOCKET_STREAM:
    return DescriptorType::Socket;
  case __WASI_FILETYPE_SYMBOLIC_LINK:
    return DescriptorType::SymbolicLink;
  default:
    return DescriptorType::Unknown;
  }
}

DescriptorStat statOf(const __wasi_filestat_t &S) noexcept {
  DescriptorStat Out;
  Out.Type = typeOf(S.filetype);
  Out.LinkCount = S.nlink;
  Out.Size = S.size;
  Out.DataAccess = Datetime::fromNanos(S.atim);
  Out.DataModification = Datetime::fromNanos(S.mtim);
  Out.StatusChange = Datetime::fromNanos(S.ctim);
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

// Every entry of the directory behind Node but `.` and `..`.
WASI::WasiExpect<std::vector<DirectoryEntry>>
entriesOf(WASI::VINode &Node) noexcept {
  std::vector<DirectoryEntry> Entries;
  std::vector<uint8_t> Buffer(4096);
  __wasi_dircookie_t Cookie = 0;
  bool More = true;
  while (More) {
    __wasi_size_t Size = 0;
    EXPECTED_TRY(Node.fdReaddir(Buffer, Cookie, Size));
    size_t Off = 0;
    bool Partial = false;
    while (Off + sizeof(__wasi_dirent_t) <= Size) {
      __wasi_dirent_t Dirent;
      std::memcpy(&Dirent, Buffer.data() + Off, sizeof(Dirent));
      if (Off + sizeof(Dirent) + Dirent.d_namlen > Size) {
        Partial = true;
        break;
      }
      std::string Name(
          reinterpret_cast<const char *>(Buffer.data() + Off + sizeof(Dirent)),
          Dirent.d_namlen);
      Cookie = Dirent.d_next;
      Off += sizeof(Dirent) + Dirent.d_namlen;
      if (Name != "." && Name != "..") {
        Entries.push_back(
            DirectoryEntry{typeOf(Dirent.d_type), std::move(Name)});
      }
    }
    if (Off == 0 && Size > 0) {
      return WASI::WasiUnexpect(__WASI_ERRNO_NAMETOOLONG);
    }
    More = Partial || Size == Buffer.size();
  }
  return Entries;
}

} // namespace

Expect<FsResult<Runtime::Component::Own<InputStream>>>
DescriptorReadViaStream::body(Runtime::Component::CallingFrame &,
                              Descriptor Self, uint64_t Offset) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<Runtime::Component::Own<InputStream>>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stream = std::make_shared<NodeInputStream>(
      Host, std::move(*Node), NodeInputStream::Mode::File, Offset);
  return FsResult<Runtime::Component::Own<InputStream>>(
      Runtime::Component::Own<InputStream>{
          Host.inputs().add(std::move(Stream))});
}

Expect<FsResult<Runtime::Component::Own<OutputStream>>>
DescriptorWriteViaStream::body(Runtime::Component::CallingFrame &,
                               Descriptor Self, uint64_t Offset) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<Runtime::Component::Own<OutputStream>>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stream = std::make_shared<NodeOutputStream>(
      Host, std::move(*Node), NodeOutputStream::Mode::File, Offset);
  return FsResult<Runtime::Component::Own<OutputStream>>(
      Runtime::Component::Own<OutputStream>{
          Host.outputs().add(std::move(Stream))});
}

Expect<FsResult<Runtime::Component::Own<OutputStream>>>
DescriptorAppendViaStream::body(Runtime::Component::CallingFrame &,
                                Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<Runtime::Component::Own<OutputStream>>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stream = std::make_shared<NodeOutputStream>(
      Host, std::move(*Node), NodeOutputStream::Mode::Append);
  return FsResult<Runtime::Component::Own<OutputStream>>(
      Runtime::Component::Own<OutputStream>{
          Host.outputs().add(std::move(Stream))});
}

Expect<FsStatus> DescriptorAdvise::body(Runtime::Component::CallingFrame &,
                                        Descriptor Self, uint64_t Offset,
                                        uint64_t Length, Advice Adv) {
  return status(Self, [&](WASI::VINode &Node) {
    return Node.fdAdvise(Offset, Length, static_cast<__wasi_advice_t>(Adv));
  });
}

Expect<FsStatus> DescriptorSync::body(Runtime::Component::CallingFrame &,
                                      Descriptor Self) {
  return status(Self, [this](WASI::VINode &Node) {
    return DataOnly ? Node.fdDatasync() : Node.fdSync();
  });
}

Expect<FsResult<DescriptorFlags>>
DescriptorGetFlags::body(Runtime::Component::CallingFrame &, Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorFlags>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  __wasi_fdstat_t Stat;
  std::memset(&Stat, 0, sizeof(Stat));
  if (auto Res = (*Node)->fdFdstatGet(Stat); !Res) {
    return FsResult<DescriptorFlags>(
        Unexpected<FsErrorCode>(errorOf(Res.error())));
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
    return FsResult<DescriptorType>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stat = statOfNode(**Node);
  if (!Stat) {
    return FsResult<DescriptorType>(
        Unexpected<FsErrorCode>(errorOf(Stat.error())));
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

Expect<FsResult<std::tuple<std::vector<uint8_t>, bool>>>
DescriptorRead::body(Runtime::Component::CallingFrame &, Descriptor Self,
                     uint64_t Length, uint64_t Offset) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<std::tuple<std::vector<uint8_t>, bool>>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  std::vector<uint8_t> Buffer(
      static_cast<size_t>(std::min<uint64_t>(Length, 1U << 20)));
  __wasi_size_t Count = 0;
  if (!Buffer.empty()) {
    std::array<Span<uint8_t>, 1> IOVs{Span<uint8_t>(Buffer)};
    if (auto Res = (*Node)->fdPread(IOVs, Offset, Count); !Res) {
      return FsResult<std::tuple<std::vector<uint8_t>, bool>>(
          Unexpected<FsErrorCode>(errorOf(Res.error())));
    }
  }
  const bool End = Count == 0 && Length > 0;
  Buffer.resize(Count);
  return FsResult<std::tuple<std::vector<uint8_t>, bool>>(
      std::make_tuple(std::move(Buffer), End));
}

Expect<FsResult<uint64_t>>
DescriptorWrite::body(Runtime::Component::CallingFrame &, Descriptor Self,
                      std::vector<uint8_t> Buffer, uint64_t Offset) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<uint64_t>(Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  std::array<Span<const uint8_t>, 1> IOVs{Span<const uint8_t>(Buffer)};
  __wasi_size_t Written = 0;
  if (auto Res = (*Node)->fdPwrite(IOVs, Offset, Written); !Res) {
    return FsResult<uint64_t>(Unexpected<FsErrorCode>(errorOf(Res.error())));
  }
  return FsResult<uint64_t>(static_cast<uint64_t>(Written));
}

Expect<FsResult<Runtime::Component::Own<DirectoryEntryStream>>>
DescriptorReadDirectory::body(Runtime::Component::CallingFrame &,
                              Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<Runtime::Component::Own<DirectoryEntryStream>>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Entries = entriesOf(**Node);
  if (!Entries) {
    return FsResult<Runtime::Component::Own<DirectoryEntryStream>>(
        Unexpected<FsErrorCode>(errorOf(Entries.error())));
  }
  auto Stream = std::make_shared<DirectoryEntryStream>();
  Stream->Entries = std::move(*Entries);
  return FsResult<Runtime::Component::Own<DirectoryEntryStream>>(
      Runtime::Component::Own<DirectoryEntryStream>{
          Directories.add(std::move(Stream))});
}

Expect<FsStatus> DescriptorPathOp::body(Runtime::Component::CallingFrame &,
                                        Descriptor Self, std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  WASI::WasiExpect<void> Res;
  switch (Which) {
  case Op::CreateDirectory:
    Res = WASI::VINode::pathCreateDirectory(*Node, Path);
    break;
  case Op::RemoveDirectory:
    Res = WASI::VINode::pathRemoveDirectory(*Node, Path);
    break;
  case Op::UnlinkFile:
    Res = WASI::VINode::pathUnlinkFile(*Node, Path);
    break;
  }
  if (!Res) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsResult<std::string>>
DescriptorReadlinkAt::body(Runtime::Component::CallingFrame &, Descriptor Self,
                           std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<std::string>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  std::vector<char> Buffer(4096);
  __wasi_size_t Read = 0;
  if (auto Res = WASI::VINode::pathReadlink(*Node, Path, Buffer, Read); !Res) {
    return FsResult<std::string>(Unexpected<FsErrorCode>(errorOf(Res.error())));
  }
  return FsResult<std::string>(std::string(Buffer.data(), Read));
}

Expect<FsResult<DescriptorStat>>
DescriptorStatFunc::body(Runtime::Component::CallingFrame &, Descriptor Self) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorStat>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stat = statOfNode(**Node);
  if (!Stat) {
    return FsResult<DescriptorStat>(
        Unexpected<FsErrorCode>(errorOf(Stat.error())));
  }
  return FsResult<DescriptorStat>(statOf(*Stat));
}

Expect<FsResult<DescriptorStat>>
DescriptorStatAt::body(Runtime::Component::CallingFrame &, Descriptor Self,
                       PathFlags Flags, std::string Path) {
  auto Node = node(Self);
  if (!Node) {
    return FsResult<DescriptorStat>(
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stat = statOfPath(*Node, Flags, Path);
  if (!Stat) {
    return FsResult<DescriptorStat>(
        Unexpected<FsErrorCode>(errorOf(Stat.error())));
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
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathFilestatSetTimes(
          *Node, Path, lookupFlagsOf(Flags), Access.nanos(),
          Modification.nanos(),
          static_cast<__wasi_fstflags_t>(Access.fstFlags(true) |
                                         Modification.fstFlags(false)));
      !Res) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Res.error())));
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
    return FsStatus(Unexpected<FsErrorCode>(FsErrorCode::BadDescriptor));
  }
  if (auto Res = WASI::VINode::pathLink(*Old, OldPath, *New, NewPath,
                                        lookupFlagsOf(Flags));
      !Res) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Res.error())));
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
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  // The preview-1 open mode follows the rights that imply reading or writing.
  uint64_t Rights = WasiP3::AllRights;
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
      static_cast<__wasi_rights_t>(Rights), WasiP3::AllRights,
      static_cast<__wasi_fdflags_t>(FdFlags));
  if (!Opened) {
    return FsResult<Runtime::Component::Own<WASI::VINode>>(
        Unexpected<FsErrorCode>(errorOf(Opened.error())));
  }
  return FsResult<Runtime::Component::Own<WASI::VINode>>(
      Runtime::Component::Own<WASI::VINode>{Table.add(std::move(*Opened))});
}

Expect<FsStatus> DescriptorRenameAt::body(Runtime::Component::CallingFrame &,
                                          Descriptor Self, std::string OldPath,
                                          Descriptor NewDescriptor,
                                          std::string NewPath) {
  auto Old = node(Self);
  auto New = node(NewDescriptor);
  if (!Old || !New) {
    return FsStatus(Unexpected<FsErrorCode>(FsErrorCode::BadDescriptor));
  }
  if (auto Res = WASI::VINode::pathRename(*Old, OldPath, *New, NewPath); !Res) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Res.error())));
  }
  return FsStatus(Runtime::Component::Unit{});
}

Expect<FsStatus> DescriptorSymlinkAt::body(Runtime::Component::CallingFrame &,
                                           Descriptor Self, std::string OldPath,
                                           std::string NewPath) {
  auto Node = node(Self);
  if (!Node) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  if (auto Res = WASI::VINode::pathSymlink(OldPath, *Node, NewPath); !Res) {
    return FsStatus(Unexpected<FsErrorCode>(errorOf(Res.error())));
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
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stat = statOfNode(**Node);
  if (!Stat) {
    return FsResult<MetadataHashValue>(
        Unexpected<FsErrorCode>(errorOf(Stat.error())));
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
        Unexpected<FsErrorCode>(errorOf(Node.error())));
  }
  auto Stat = statOfPath(*Node, Flags, Path);
  if (!Stat) {
    return FsResult<MetadataHashValue>(
        Unexpected<FsErrorCode>(errorOf(Stat.error())));
  }
  return FsResult<MetadataHashValue>(
      MetadataHashValue::of(Stat->dev, Stat->ino));
}

Expect<FsResult<std::optional<DirectoryEntry>>> ReadDirectoryEntry::body(
    Runtime::Component::CallingFrame &,
    Runtime::Component::Borrow<DirectoryEntryStream> Self) {
  auto Stream = Directories.get(Self.Rep);
  if (!Stream) {
    return FsResult<std::optional<DirectoryEntry>>(
        Unexpected<FsErrorCode>(FsErrorCode::BadDescriptor));
  }
  if (Stream->Next >= Stream->Entries.size()) {
    return FsResult<std::optional<DirectoryEntry>>(
        std::optional<DirectoryEntry>());
  }
  return FsResult<std::optional<DirectoryEntry>>(
      std::optional<DirectoryEntry>(Stream->Entries[Stream->Next++]));
}

Expect<std::optional<FsErrorCode>>
FilesystemErrorCode::body(Runtime::Component::CallingFrame &, ErrorBorrow Err) {
  auto Error = Host.errors().get(Err.Rep);
  if (!Error) {
    return std::optional<FsErrorCode>();
  }
  return std::optional<FsErrorCode>(errorOf(Error->Errno));
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

FilesystemTypesInstance::FilesystemTypesInstance(IoHost &Host)
    : ComponentInstance("wasi:filesystem/types@0.2.12") {
  exportType("input-stream",
             addSharedResourceType<InputStream>(Host.InputType));
  exportType("output-stream",
             addSharedResourceType<OutputStream>(Host.OutputType));
  exportType("error", addSharedResourceType<IoError>(Host.ErrorType));
  exportType("descriptor",
             addHostResourceType<WASI::VINode>(
                 [this](uint64_t Rep) { Descriptors.remove(Rep); }));
  exportType("directory-entry-stream",
             addHostResourceType<DirectoryEntryStream>(
                 [this](uint64_t Rep) { Directories.remove(Rep); }));
  const auto Mint = getTypeMinter();
  exportType("datetime",
             Runtime::Component::Wit<Datetime>::type(Mint).getTypeIndex());
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
             Runtime::Component::Wit<FsErrorCode>::type(Mint).getTypeIndex());
  exportType("advice",
             Runtime::Component::Wit<Advice>::type(Mint).getTypeIndex());
  exportType(
      "metadata-hash-value",
      Runtime::Component::Wit<MetadataHashValue>::type(Mint).getTypeIndex());

  auto &D = Descriptors;
  addHostFunc("[method]descriptor.read-via-stream",
              std::make_unique<DescriptorReadViaStream>(Host, D));
  addHostFunc("[method]descriptor.write-via-stream",
              std::make_unique<DescriptorWriteViaStream>(Host, D));
  addHostFunc("[method]descriptor.append-via-stream",
              std::make_unique<DescriptorAppendViaStream>(Host, D));
  addHostFunc("[method]descriptor.advise",
              std::make_unique<DescriptorAdvise>(Host, D));
  addHostFunc("[method]descriptor.sync-data",
              std::make_unique<DescriptorSync>(Host, D, true));
  addHostFunc("[method]descriptor.get-flags",
              std::make_unique<DescriptorGetFlags>(Host, D));
  addHostFunc("[method]descriptor.get-type",
              std::make_unique<DescriptorGetType>(Host, D));
  addHostFunc("[method]descriptor.set-size",
              std::make_unique<DescriptorSetSize>(Host, D));
  addHostFunc("[method]descriptor.set-times",
              std::make_unique<DescriptorSetTimes>(Host, D));
  addHostFunc("[method]descriptor.read",
              std::make_unique<DescriptorRead>(Host, D));
  addHostFunc("[method]descriptor.write",
              std::make_unique<DescriptorWrite>(Host, D));
  addHostFunc("[method]descriptor.read-directory",
              std::make_unique<DescriptorReadDirectory>(Host, D, Directories));
  addHostFunc("[method]descriptor.sync",
              std::make_unique<DescriptorSync>(Host, D, false));
  addHostFunc("[method]descriptor.create-directory-at",
              std::make_unique<DescriptorPathOp>(
                  Host, D, DescriptorPathOp::Op::CreateDirectory));
  addHostFunc("[method]descriptor.stat",
              std::make_unique<DescriptorStatFunc>(Host, D));
  addHostFunc("[method]descriptor.stat-at",
              std::make_unique<DescriptorStatAt>(Host, D));
  addHostFunc("[method]descriptor.set-times-at",
              std::make_unique<DescriptorSetTimesAt>(Host, D));
  addHostFunc("[method]descriptor.link-at",
              std::make_unique<DescriptorLinkAt>(Host, D));
  addHostFunc("[method]descriptor.open-at",
              std::make_unique<DescriptorOpenAt>(Host, D));
  addHostFunc("[method]descriptor.readlink-at",
              std::make_unique<DescriptorReadlinkAt>(Host, D));
  addHostFunc("[method]descriptor.remove-directory-at",
              std::make_unique<DescriptorPathOp>(
                  Host, D, DescriptorPathOp::Op::RemoveDirectory));
  addHostFunc("[method]descriptor.rename-at",
              std::make_unique<DescriptorRenameAt>(Host, D));
  addHostFunc("[method]descriptor.symlink-at",
              std::make_unique<DescriptorSymlinkAt>(Host, D));
  addHostFunc("[method]descriptor.unlink-file-at",
              std::make_unique<DescriptorPathOp>(
                  Host, D, DescriptorPathOp::Op::UnlinkFile));
  addHostFunc("[method]descriptor.is-same-object",
              std::make_unique<DescriptorIsSameObject>(Host, D));
  addHostFunc("[method]descriptor.metadata-hash",
              std::make_unique<DescriptorMetadataHash>(Host, D));
  addHostFunc("[method]descriptor.metadata-hash-at",
              std::make_unique<DescriptorMetadataHashAt>(Host, D));
  addHostFunc("[method]directory-entry-stream.read-directory-entry",
              std::make_unique<ReadDirectoryEntry>(Directories));
  addHostFunc("filesystem-error-code",
              std::make_unique<FilesystemErrorCode>(Host));
}

PreopensInstance::PreopensInstance(WasiComponent::Env &Env,
                                   FilesystemTypesInstance &Types)
    : ComponentInstance("wasi:filesystem/preopens@0.2.12") {
  exportType("descriptor", addSharedResourceType<WASI::VINode>(
                               Types.findTypeResource("descriptor")));
  addHostFunc("get-directories",
              std::make_unique<GetDirectories>(Env, Types.descriptors()));
}

std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>>
makeFilesystemInstances(WasiComponent::Env &Env, IoHost &Host) {
  std::vector<std::unique_ptr<Runtime::Instance::ComponentInstance>> Out;
  auto Types = std::make_unique<FilesystemTypesInstance>(Host);
  auto Preopens = std::make_unique<PreopensInstance>(Env, *Types);
  Out.push_back(std::move(Types));
  Out.push_back(std::move(Preopens));
  return Out;
}

} // namespace WasiP2
} // namespace Host
} // namespace WasmEdge
