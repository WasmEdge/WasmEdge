// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#pragma once

#include "host/wasi/wasibase.h"

#include <cstdint>

namespace WasmEdge {
namespace Host {

class WasiArgsGet : public Wasi<WasiArgsGet> {
public:
  WasiArgsGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ArgvPtr,
                        uint32_t ArgvBufPtr);
};

class WasiArgsSizesGet : public Wasi<WasiArgsSizesGet> {
public:
  WasiArgsSizesGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t /* Out */ ArgcPtr,
                        uint32_t /* Out */ ArgvBufSizePtr);
};

class WasiEnvironGet : public Wasi<WasiEnvironGet> {
public:
  WasiEnvironGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t EnvPtr,
                        uint32_t EnvBufPtr);
};

class WasiEnvironSizesGet : public Wasi<WasiEnvironSizesGet> {
public:
  WasiEnvironSizesGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t /* Out */ EnvCntPtr,
                        uint32_t /* Out */ EnvBufSizePtr);
};

class WasiClockResGet : public Wasi<WasiClockResGet> {
public:
  WasiClockResGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ClockId,
                        uint32_t /* Out */ ResolutionPtr);
};

class WasiClockTimeGet : public Wasi<WasiClockTimeGet> {
public:
  WasiClockTimeGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t ClockId,
                        uint64_t Precision, uint32_t /* Out */ TimePtr);
};

class WasiFdAdvise : public Wasi<WasiFdAdvise> {
public:
  WasiFdAdvise(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint64_t Offset, uint64_t Len, uint32_t Advice);
};

class WasiFdAllocate : public Wasi<WasiFdAllocate> {
public:
  WasiFdAllocate(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint64_t Offset, uint64_t Len);
};

class WasiFdClose : public Wasi<WasiFdClose> {
public:
  WasiFdClose(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd);
};

class WasiFdDatasync : public Wasi<WasiFdDatasync> {
public:
  WasiFdDatasync(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd);
};

class WasiFdFdstatGet : public Wasi<WasiFdFdstatGet> {
public:
  WasiFdFdstatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t /* Out */ FdStatPtr);
};

class WasiFdFdstatSetFlags : public Wasi<WasiFdFdstatSetFlags> {
public:
  WasiFdFdstatSetFlags(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t FsFlags);
};

class WasiFdFdstatSetRights : public Wasi<WasiFdFdstatSetRights> {
public:
  WasiFdFdstatSetRights(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint64_t FsRightsBase, uint64_t FsRightsInheriting);
};

class WasiFdFilestatGet : public Wasi<WasiFdFilestatGet> {
public:
  WasiFdFilestatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t /* Out */ FilestatPtr);
};

class WasiFdFilestatSetSize : public Wasi<WasiFdFilestatSetSize> {
public:
  WasiFdFilestatSetSize(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint64_t Size);
};

class WasiFdFilestatSetTimes : public Wasi<WasiFdFilestatSetTimes> {
public:
  WasiFdFilestatSetTimes(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint64_t ATim, uint64_t MTim, uint32_t FstFlags);
};

class WasiFdPread : public Wasi<WasiFdPread> {
public:
  WasiFdPread(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t IOVsPtr, uint32_t IOVsLen, uint64_t Offset,
                        uint32_t /* Out */ NReadPtr);
};

class WasiFdPrestatGet : public Wasi<WasiFdPrestatGet> {
public:
  WasiFdPrestatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t /* Out */ PreStatPtr);
};

class WasiFdPrestatDirName : public Wasi<WasiFdPrestatDirName> {
public:
  WasiFdPrestatDirName(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t PathBufPtr, uint32_t PathLen);
};

class WasiFdPwrite : public Wasi<WasiFdPwrite> {
public:
  WasiFdPwrite(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSLen, uint64_t Offset,
                        uint32_t /* Out */ NWrittenPtr);
};

class WasiFdRead : public Wasi<WasiFdRead> {
public:
  WasiFdRead(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSLen,
                        uint32_t /* Out */ NReadPtr);
};

class WasiFdReadDir : public Wasi<WasiFdReadDir> {
public:
  WasiFdReadDir(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t BufPtr, uint32_t BufLen, uint64_t Cookie,
                        uint32_t /* Out */ NReadPtr);
};

class WasiFdRenumber : public Wasi<WasiFdRenumber> {
public:
  WasiFdRenumber(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        int32_t ToFd);
};

class WasiFdSeek : public Wasi<WasiFdSeek> {
public:
  WasiFdSeek(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<int32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                       int64_t Offset, uint32_t Whence,
                       uint32_t /* Out */ NewOffsetPtr);
};

class WasiFdSync : public Wasi<WasiFdSync> {
public:
  WasiFdSync(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd);
};

class WasiFdTell : public Wasi<WasiFdTell> {
public:
  WasiFdTell(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t /* Out */ OffsetPtr);
};

class WasiFdWrite : public Wasi<WasiFdWrite> {
public:
  WasiFdWrite(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSLen,
                        uint32_t /* Out */ NWrittenPtr);
};

class WasiPathCreateDirectory : public Wasi<WasiPathCreateDirectory> {
public:
  WasiPathCreateDirectory(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPathFilestatGet : public Wasi<WasiPathFilestatGet> {
public:
  WasiPathFilestatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t Flags, uint32_t PathPtr, uint32_t PathLen,
                        uint32_t /* Out */ FilestatPtr);
};

class WasiPathFilestatSetTimes : public Wasi<WasiPathFilestatSetTimes> {
public:
  WasiPathFilestatSetTimes(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t Flags, uint32_t PathPtr, uint32_t PathLen,
                        uint64_t ATim, uint64_t MTim, uint32_t FstFlags);
};

class WasiPathLink : public Wasi<WasiPathLink> {
public:
  WasiPathLink(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t OldFd,
                        uint32_t OldFlags, uint32_t OldPathPtr,
                        uint32_t OldPathLen, int32_t NewFd, uint32_t NewPathPtr,
                        uint32_t NewPathLen);
};

class WasiPathOpen : public Wasi<WasiPathOpen> {
public:
  WasiPathOpen(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t DirFd,
                        uint32_t DirFlags, uint32_t PathPtr, uint32_t PathLen,
                        uint32_t OFlags, uint64_t FsRightsBase,
                        uint64_t FsRightsInheriting, uint32_t FsFlags,
                        uint32_t /* Out */ FdPtr);
};

class WasiPathReadLink : public Wasi<WasiPathReadLink> {
public:
  WasiPathReadLink(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen, uint32_t BufPtr,
                        uint32_t BufLen, uint32_t /* Out */ NReadPtr);
};

class WasiPathRemoveDirectory : public Wasi<WasiPathRemoveDirectory> {
public:
  WasiPathRemoveDirectory(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPathRename : public Wasi<WasiPathRename> {
public:
  WasiPathRename(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t OldPathPtr, uint32_t OldPathLen, int32_t NewFd,
                        uint32_t NewPathPtr, uint32_t NewPathLen);
};

class WasiPathSymlink : public Wasi<WasiPathSymlink> {
public:
  WasiPathSymlink(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t OldPathPtr,
                        uint32_t OldPathLen, int32_t Fd, uint32_t NewPathPtr,
                        uint32_t NewPathLen);
};

class WasiPathUnlinkFile : public Wasi<WasiPathUnlinkFile> {
public:
  WasiPathUnlinkFile(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPollOneoff : public Wasi<WasiPollOneoff> {
public:
  WasiPollOneoff(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t InPtr,
                        uint32_t OutPtr, uint32_t NSubscriptions,
                        uint32_t /* Out */ NEventsPtr);
};

class WasiEpollOneoff : public Wasi<WasiEpollOneoff> {
public:
  WasiEpollOneoff(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t InPtr,
                        uint32_t OutPtr, uint32_t NSubscriptions,
                        uint32_t /* Out */ NEventsPtr);
};

class WasiProcExit : public Wasi<WasiProcExit> {
public:
  WasiProcExit(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<void> body(const Runtime::CallingFrame &Frame, uint32_t Status);
};

class WasiProcRaise : public Wasi<WasiProcRaise> {
public:
  WasiProcRaise(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t Signal);
};

class WasiSchedYield : public Wasi<WasiSchedYield> {
public:
  WasiSchedYield(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame);
};

class WasiRandomGet : public Wasi<WasiRandomGet> {
public:
  WasiRandomGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t BufPtr,
                        uint32_t BufLen);
};

class WasiSockOpen : public Wasi<WasiSockOpen> {
public:
  WasiSockOpen(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame,
                        uint32_t AddressFamily, uint32_t SockType,
                        uint32_t /* Out */ RoFdPtr);
};

class WasiSockBind : public Wasi<WasiSockBind> {
public:
  WasiSockBind(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t AddressPtr, uint32_t Port);
};

class WasiSockListen : public Wasi<WasiSockListen> {
public:
  WasiSockListen(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        int32_t Backlog);
};

class WasiSockAccept : public Wasi<WasiSockAccept> {
public:
  WasiSockAccept(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t /* Out */ RoFdPtr);
};

class WasiSockConnect : public Wasi<WasiSockConnect> {
public:
  WasiSockConnect(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t AddressPtr, uint32_t Port);
};

class WasiSockRecv : public Wasi<WasiSockRecv> {
public:
  WasiSockRecv(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t RiDataPtr, uint32_t RiDataLen,
                        uint32_t RiFlags, uint32_t /* Out */ RoDataLenPtr,
                        uint32_t /* Out */ RoFlagsPtr);
};

class WasiSockRecvFrom : public Wasi<WasiSockRecvFrom> {
public:
  WasiSockRecvFrom(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t RiDataPtr, uint32_t RiDataLen,
                        uint32_t AddressPtr, uint32_t RiFlags,
                        uint32_t /* Out */ RoDataLenPtr,
                        uint32_t /* Out */ RoFlagsPtr);
};

class WasiSockSend : public Wasi<WasiSockSend> {
public:
  WasiSockSend(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t SiDataPtr, uint32_t SiDataLen,
                        uint32_t SiFlags, uint32_t /* Out */ SoDataLenPtr);
};

class WasiSockSendTo : public Wasi<WasiSockSendTo> {
public:
  WasiSockSendTo(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t SiDataPtr, uint32_t SiDataLen,
                        uint32_t AddressPtr, int32_t Port, uint32_t SiFlags,
                        uint32_t /* Out */ SoDataLenPtr);
};

class WasiSockShutdown : public Wasi<WasiSockShutdown> {
public:
  WasiSockShutdown(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t SdFlags);
};

class WasiSockGetOpt : public Wasi<WasiSockGetOpt> {
public:
  WasiSockGetOpt(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t SockOptLevel, uint32_t SockOptName,
                        uint32_t FlagPtr, uint32_t FlagSizePtr);
};

class WasiSockSetOpt : public Wasi<WasiSockSetOpt> {
public:
  WasiSockSetOpt(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t SockOptLevel, uint32_t SockOptName,
                        uint32_t FlagPtr, uint32_t FlagSizePtr);
};

class WasiSockGetLocalAddr : public Wasi<WasiSockGetLocalAddr> {
public:
  WasiSockGetLocalAddr(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t AddressPtr, uint32_t AddressTypePtr,
                        uint32_t PortPtr);
};

class WasiSockGetPeerAddr : public Wasi<WasiSockGetPeerAddr> {
public:
  WasiSockGetPeerAddr(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, int32_t Fd,
                        uint32_t AddressPtr, uint32_t AddressTypePtr,
                        uint32_t PortPtr);
};

class WasiGetAddrinfo : public Wasi<WasiGetAddrinfo> {
public:
  WasiGetAddrinfo(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(const Runtime::CallingFrame &Frame, uint32_t NodePtr,
                        uint32_t NodeLen, uint32_t ServicePtr,
                        uint32_t ServiceLen, uint32_t HintsPtr, uint32_t ResPtr,
                        uint32_t MaxResLength, uint32_t ResLengthPtr);
};

} // namespace Host
} // namespace WasmEdge
