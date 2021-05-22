// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/span.h"
#include "host/wasi/vfs.h"
#include "wasi/api.hpp"
#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>

namespace WasmEdge {
namespace Host {

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdAdvise(__wasi_filesize_t Offset, __wasi_filesize_t Len,
                              __wasi_advice_t Advice) noexcept {
  switch (Advice) {
  case __WASI_ADVICE_NORMAL:
  case __WASI_ADVICE_SEQUENTIAL:
  case __WASI_ADVICE_RANDOM:
  case __WASI_ADVICE_WILLNEED:
  case __WASI_ADVICE_DONTNEED:
  case __WASI_ADVICE_NOREUSE:
    break;
  default:
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_ADVISE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdAdvise(Offset, Len, Advice);
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdAllocate(__wasi_filesize_t Offset,
                                __wasi_filesize_t Len) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_ALLOCATE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdAllocate(Offset, Len);
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdClose() noexcept {
  if (unlikely(!isPreopened())) {
    return __WASI_ERRNO_NOTSUP;
  }

  return static_cast<ImplT *>(this)->doFdClose();
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdDatasync() noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_DATASYNC))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdDatasync();
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdFdstatGet(GetterT<__wasi_fdstat_t *> Getter) noexcept {
  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [FdStatPtr] = *Res;
    if (auto Error = static_cast<ImplT *>(this)->doFdFdstatGet(*FdStatPtr);
        unlikely(Error != __WASI_ERRNO_SUCCESS)) {
      return Error;
    }
    /// 1. __wasi_fdstat_t.fs_rights_base
    FdStatPtr->fs_rights_base = Rights;

    /// 2. __wasi_fdstat_t.fs_rights_inheriting
    FdStatPtr->fs_rights_inheriting = InheritingRights;

    return __WASI_ERRNO_SUCCESS;
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdFdstatSetFlags(__wasi_fdflags_t FdFlags) noexcept {
  if (unlikely(!isValidFlags(FdFlags))) {
    return __WASI_ERRNO_INVAL;
  }
  __wasi_rights_t RequiredRights = __WASI_RIGHTS_FD_FDSTAT_SET_FLAGS;
  if (FdFlags & __WASI_FDFLAGS_DSYNC) {
    RequiredRights |= __WASI_RIGHTS_FD_DATASYNC;
  }
  if (FdFlags & __WASI_FDFLAGS_RSYNC) {
    RequiredRights |= __WASI_RIGHTS_FD_SYNC;
  }
  if (FdFlags & __WASI_FDFLAGS_SYNC) {
    RequiredRights |= __WASI_RIGHTS_FD_SYNC;
  }
  if (unlikely(!checkRights(RequiredRights))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdFdstatSetFlags(FdFlags);
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdFdstatSetRights(
    __wasi_rights_t FsRightsBase, __wasi_rights_t FsRightsInheriting) noexcept {
  if (unlikely(!isValidFlags(FsRightsBase | FsRightsInheriting))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(FsRightsBase, FsRightsInheriting))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  Rights = FsRightsBase;
  InheritingRights = FsRightsInheriting;

  return {};
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdFilestatGet(
    GetterT<__wasi_filestat_t *> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_FILESTAT_GET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [FilestatPtr] = *Res;
    return static_cast<ImplT *>(this)->doFdFilestatGet(*FilestatPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdFilestatSetSize(__wasi_filesize_t Size) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_FILESTAT_SET_SIZE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdFilestatSetSize(Size);
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdFilestatSetTimes(__wasi_timestamp_t ATim,
                                        __wasi_timestamp_t MTim,
                                        __wasi_fstflags_t FstFlags) noexcept {
  if (unlikely(!isValidFlags(FstFlags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_FILESTAT_SET_TIMES))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdFilestatSetTimes(ATim, MTim, FstFlags);
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdPread(
    GetterT<Span<Span<uint8_t>>, __wasi_size_t *> Getter,
    __wasi_filesize_t Offset) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_READ | __WASI_RIGHTS_FD_SEEK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [IOVS, NReadPtr] = *Res;
    cappingIOVS(IOVS);
    return static_cast<ImplT *>(this)->doFdPread(IOVS, Offset, *NReadPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdPrestatGet(GetterT<__wasi_prestat_t *> Getter) noexcept {
  if (unlikely(!isPreopened())) {
    return __WASI_ERRNO_BADF;
  }
  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [PreStatPtr] = *Res;
    PreStatPtr->tag = __WASI_PREOPENTYPE_DIR;
    PreStatPtr->u.dir.pr_name_len = PreopenName.size();
    return __WASI_ERRNO_SUCCESS;
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdPrestatDirName(GetterT<Span<uint8_t>> Getter) noexcept {
  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path] = *Res;
    const auto Size = std::min(PreopenName.size(), Path.size());
    std::copy_n(PreopenName.begin(), Size, Path.begin());
    return __WASI_ERRNO_SUCCESS;
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdPwrite(
    GetterT<Span<Span<const uint8_t>>, __wasi_size_t *> Getter,
    __wasi_filesize_t Offset) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_WRITE | __WASI_RIGHTS_FD_SEEK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [IOVS, NWrittenPtr] = *Res;
    cappingIOVS(IOVS);
    return static_cast<ImplT *>(this)->doFdPwrite(IOVS, Offset, *NWrittenPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdRead(
    GetterT<Span<Span<uint8_t>>, __wasi_size_t *> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_READ))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [IOVS, NReadPtr] = *Res;
    cappingIOVS(IOVS);
    return static_cast<ImplT *>(this)->doFdRead(IOVS, *NReadPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdReaddir(GetterT<Span<uint8_t>, __wasi_size_t *> Getter,
                               __wasi_dircookie_t Cookie) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_READDIR))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Buf, BufusedPtr] = *Res;
    return static_cast<ImplT *>(this)->doFdReaddir(Buf, Cookie, *BufusedPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdSeek(GetterT<__wasi_filedelta_t *> Getter,
                            __wasi_filedelta_t Offset,
                            __wasi_whence_t Whence) noexcept {
  if (unlikely(!isValidEnum(Whence))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_SEEK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [NewOffsetPtr] = *Res;
    return static_cast<ImplT *>(this)->doFdSeek(Offset, Whence, *NewOffsetPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdSync() noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_SYNC))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doFdSync();
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::fdTell(GetterT<__wasi_filesize_t *> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_TELL))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [OffsetPtr] = *Res;
    return static_cast<ImplT *>(this)->doFdTell(*OffsetPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::fdWrite(
    GetterT<Span<Span<const uint8_t>>, __wasi_size_t *> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_WRITE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [IOVS, NWrittenPtr] = *Res;
    cappingIOVS(IOVS);
    return static_cast<ImplT *>(this)->doFdWrite(IOVS, *NWrittenPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathCreateDirectory(
    GetterT<Span<const uint8_t>> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_CREATE_DIRECTORY))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path] = *Res;
    return static_cast<ImplT *>(this)->doPathCreateDirectory(Path);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathFilestatGet(
    GetterT<Span<const uint8_t>, __wasi_filestat_t *> Getter,
    __wasi_lookupflags_t Flags) noexcept {
  if (unlikely(!isValidFlags(Flags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_FILESTAT_GET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path, BufPtr] = *Res;
    return static_cast<ImplT *>(this)->doPathFilestatGet(Flags, Path, *BufPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathFilestatSetTimes(
    GetterT<Span<const uint8_t>> Getter, __wasi_lookupflags_t Flags,
    __wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
    __wasi_fstflags_t FstFlags) noexcept {
  if (unlikely(!isValidFlags(Flags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!isValidFlags(FstFlags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_FILESTAT_SET_TIMES))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path] = *Res;
    return static_cast<ImplT *>(this)->doPathFilestatSetTimes(Flags, Path, ATim,
                                                              MTim, FstFlags);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathLink(
    GetterT<Span<const uint8_t>, Span<const uint8_t>> Getter, ImplT &Old,
    __wasi_lookupflags_t OldFlags, ImplT &New) noexcept {
  if (unlikely(!isValidFlags(OldFlags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!Old.checkRights(__WASI_RIGHTS_PATH_LINK_SOURCE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }
  if (unlikely(!New.checkRights(__WASI_RIGHTS_PATH_LINK_TARGET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [OldPath, NewPath] = *Res;
    return ImplT::doPathLink(Old, OldFlags, OldPath, New, NewPath);
  }
}

template <typename ImplT>
inline WasiExpect<ImplT> WasiFileBase<ImplT>::pathOpen(
    GetterT<Span<const uint8_t>> Getter, __wasi_lookupflags_t DirFlags,
    __wasi_oflags_t OFlags, __wasi_rights_t FsRightsBase,
    __wasi_rights_t FsRightsInheriting, __wasi_fdflags_t FdFlags) noexcept {
  if (unlikely(!isValidFlags(DirFlags))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  if (unlikely(!isValidFlags(OFlags))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  if (unlikely(!isValidFlags(FsRightsBase | FsRightsInheriting))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }
  if (unlikely(!isValidFlags(FdFlags))) {
    return WasiUnexpect(__WASI_ERRNO_INVAL);
  }

  __wasi_rights_t RequiredRights = __WASI_RIGHTS_PATH_OPEN;
  __wasi_rights_t RequiredInheritingRights = FsRightsBase | FsRightsInheriting;
  if (OFlags & __WASI_OFLAGS_CREAT) {
    RequiredRights |= __WASI_RIGHTS_PATH_CREATE_FILE;
  }
  if (OFlags & __WASI_OFLAGS_TRUNC) {
    RequiredRights |= __WASI_RIGHTS_PATH_FILESTAT_SET_SIZE;
  }
  if (unlikely(!checkRights(RequiredRights, RequiredInheritingRights))) {
    return WasiUnexpect(__WASI_ERRNO_NOTCAPABLE);
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return WasiUnexpect(Res);
  } else {
    auto [Path] = *Res;
    return static_cast<ImplT *>(this)->doPathOpen(
        Path, DirFlags, OFlags, FsRightsBase, FsRightsInheriting, FdFlags);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathReadlink(
    GetterT<Span<const uint8_t>, Span<uint8_t>, __wasi_size_t *>
        Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_READLINK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path, Buf, BufUsedPtr] = *Res;
    return static_cast<ImplT *>(this)->doPathReadlink(Path, Buf, *BufUsedPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathRemoveDirectory(
    GetterT<Span<const uint8_t>> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_REMOVE_DIRECTORY))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path] = *Res;
    return static_cast<ImplT *>(this)->doPathRemoveDirectory(Path);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathRename(
    GetterT<Span<const uint8_t>, Span<const uint8_t>> Getter, ImplT &Old,
    ImplT &New) noexcept {
  if (unlikely(!Old.checkRights(__WASI_RIGHTS_PATH_RENAME_SOURCE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }
  if (unlikely(!New.checkRights(__WASI_RIGHTS_PATH_RENAME_TARGET))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [OldPath, NewPath] = *Res;
    return ImplT::doPathRename(Old, OldPath, New, NewPath);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathSymlink(
    GetterT<Span<const uint8_t>, Span<const uint8_t>> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_SYMLINK))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [OldPath, NewPath] = *Res;
    return static_cast<ImplT *>(this)->doPathSymlink(OldPath, NewPath);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::pathUnlinkFile(
    GetterT<Span<const uint8_t>> Getter) noexcept {
  if (unlikely(!checkRights(__WASI_RIGHTS_PATH_UNLINK_FILE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [Path] = *Res;
    return static_cast<ImplT *>(this)->doPathUnlinkFile(Path);
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::pollOneoff(GetterT<Span<const __wasi_subscription_t>,
                                        Span<__wasi_event_t>, __wasi_size_t *>
                                    Getter,
                                std::function<GetterR<Span<const Subscription>>(
                                    Span<const __wasi_subscription_t>)>
                                    SubscriptionTranslator) noexcept {
  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [In, Out, NEventsPtr] = *Res;
    const auto EventArray = Out;
    auto &NEvents = *NEventsPtr;

    for (const __wasi_subscription_t &Sub : In) {
      if (unlikely(!WasiFileBase::isValidEnum(Sub.u.tag))) {
        return __WASI_ERRNO_INVAL;
      }
      if (Sub.u.tag == __WASI_EVENTTYPE_CLOCK) {
        if (unlikely(!WasiFileBase::isValidEnum(Sub.u.u.clock.id))) {
          return __WASI_ERRNO_INVAL;
        }
        if (unlikely(!WasiFileBase::isValidFlags(Sub.u.u.clock.flags))) {
          return __WASI_ERRNO_INVAL;
        }
      }
    }

    if (auto Res = SubscriptionTranslator(In); unlikely(!Res)) {
      return Res.error();
    } else {
      const auto [SubscriptionArray] = *Res;

      typename ImplT::Poller Poll;
      if (auto Err = Poll.create(EventArray.size());
          unlikely(Err != __WASI_ERRNO_SUCCESS)) {
        return Err;
      }

      NEvents = 0;

      auto RecordClockError = [&EventArray, &NEvents](const Subscription &Sub,
                                                      __wasi_errno_t Error) {
        EventArray[NEvents].userdata = Sub.Userdata;
        EventArray[NEvents].error = Error;
        EventArray[NEvents].type = Sub.Tag;
        ++NEvents;
      };

      auto RecordFdError = [&EventArray, &NEvents](const Subscription &Sub,
                                                   __wasi_errno_t Error) {
        EventArray[NEvents].userdata = Sub.Userdata;
        EventArray[NEvents].error = Error;
        EventArray[NEvents].type = Sub.Tag;
        EventArray[NEvents].fd_readwrite.nbytes = 0;
        EventArray[NEvents].fd_readwrite.flags =
            static_cast<__wasi_eventrwflags_t>(0);
        ++NEvents;
      };

      auto RecordClock = [&EventArray, &NEvents](const Subscription &Sub) {
        EventArray[NEvents].userdata = Sub.Userdata;
        EventArray[NEvents].error = __WASI_ERRNO_SUCCESS;
        EventArray[NEvents].type = Sub.Tag;
        ++NEvents;
      };

      auto RecordFd = [&EventArray, &NEvents](const Subscription &Sub,
                                              __wasi_filesize_t NBytes,
                                              __wasi_eventrwflags_t Flags) {
        EventArray[NEvents].userdata = Sub.Userdata;
        EventArray[NEvents].error = __WASI_ERRNO_SUCCESS;
        EventArray[NEvents].type = Sub.Tag;
        EventArray[NEvents].fd_readwrite.nbytes = NBytes;
        EventArray[NEvents].fd_readwrite.flags = Flags;
        ++NEvents;
      };

      for (const auto &Sub : SubscriptionArray) {
        switch (Sub.Tag) {
        case __WASI_EVENTTYPE_CLOCK:
          if (unlikely(!isValidEnum(Sub.Clock.id))) {
            RecordClockError(Sub, __WASI_ERRNO_INVAL);
            continue;
          }
          if (auto Err = Poll.addTimer(Sub);
              unlikely(Err != __WASI_ERRNO_SUCCESS)) {
            RecordClockError(Sub, Err);
            continue;
          }
          continue;
        case __WASI_EVENTTYPE_FD_READ:
          if (unlikely(!Sub.FdRead->checkRights(
                  __WASI_RIGHTS_POLL_FD_READWRITE | __WASI_RIGHTS_FD_READ))) {
            RecordFdError(Sub, __WASI_ERRNO_NOTCAPABLE);
            continue;
          }
          if (auto Err = Poll.addFdRead(Sub);
              unlikely(Err != __WASI_ERRNO_SUCCESS)) {
            RecordFdError(Sub, Err);
            continue;
          }
          break;
        case __WASI_EVENTTYPE_FD_WRITE:
          if (unlikely(!Sub.FdRead->checkRights(
                  __WASI_RIGHTS_POLL_FD_READWRITE | __WASI_RIGHTS_FD_WRITE))) {
            RecordFdError(Sub, __WASI_ERRNO_NOTCAPABLE);
            continue;
          }
          if (auto Err = Poll.addFdWrite(Sub);
              unlikely(Err != __WASI_ERRNO_SUCCESS)) {
            RecordFdError(Sub, Err);
            continue;
          }
          break;
        }
      }

      return Poll.wait(RecordClock, RecordFd);
    }
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::sockRecv(
    GetterT<Span<Span<uint8_t>>, __wasi_size_t *, __wasi_roflags_t *> Getter,
    __wasi_riflags_t RiFlags) noexcept {
  if (unlikely(!isValidFlags(RiFlags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_READ))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [RiData, RoDatalenPtr, RoFlagsPtr] = *Res;
    cappingIOVS(RiData);
    return static_cast<ImplT *>(this)->doSockRecv(RiData, RiFlags,
                                                  *RoDatalenPtr, *RoFlagsPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t WasiFileBase<ImplT>::sockSend(
    GetterT<Span<Span<const uint8_t>>, __wasi_size_t *> Getter,
    __wasi_siflags_t SiFlags) noexcept {
  if (unlikely(!isValidFlags(SiFlags))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_FD_WRITE))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  if (auto Res = Getter(); unlikely(!Res)) {
    return Res.error();
  } else {
    auto [SiData, SoDatalenPtr] = *Res;
    cappingIOVS(SiData);
    return static_cast<ImplT *>(this)->doSockSend(SiData, SiFlags,
                                                  *SoDatalenPtr);
  }
}

template <typename ImplT>
inline __wasi_errno_t
WasiFileBase<ImplT>::sockShutdown(__wasi_sdflags_t How) noexcept {
  if (unlikely(!isValidFlags(How))) {
    return __WASI_ERRNO_INVAL;
  }
  if (unlikely(!checkRights(__WASI_RIGHTS_SOCK_SHUTDOWN))) {
    return __WASI_ERRNO_NOTCAPABLE;
  }

  return static_cast<ImplT *>(this)->doSockShutdown(How);
}

} // namespace Host
} // namespace WasmEdge
