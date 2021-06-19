// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "host/wasi/wasibase.h"

namespace WasmEdge {
namespace Host {

class WasiArgsGet : public Wasi<WasiArgsGet> {
public:
  WasiArgsGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ArgvPtr, uint32_t ArgvBufPtr);
};

class WasiArgsSizesGet : public Wasi<WasiArgsSizesGet> {
public:
  WasiArgsSizesGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

class WasiEnvironGet : public Wasi<WasiEnvironGet> {
public:
  WasiEnvironGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t EnvPtr, uint32_t EnvBufPtr);
};

class WasiEnvironSizesGet : public Wasi<WasiEnvironSizesGet> {
public:
  WasiEnvironSizesGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t EnvCntPtr, uint32_t EnvBufSizePtr);
};

class WasiClockResGet : public Wasi<WasiClockResGet> {
public:
  WasiClockResGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ClockId, uint32_t ResolutionPtr);
};

class WasiClockTimeGet : public Wasi<WasiClockTimeGet> {
public:
  WasiClockTimeGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ClockId, uint64_t Precision, uint32_t TimePtr);
};

class WasiFdAdvise : public Wasi<WasiFdAdvise> {
public:
  WasiFdAdvise(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t Offset, uint64_t Len, uint32_t Advice);
};

class WasiFdAllocate : public Wasi<WasiFdAllocate> {
public:
  WasiFdAllocate(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t Offset, uint64_t Len);
};

class WasiFdClose : public Wasi<WasiFdClose> {
public:
  WasiFdClose(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd);
};

class WasiFdDatasync : public Wasi<WasiFdDatasync> {
public:
  WasiFdDatasync(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd);
};

class WasiFdFdstatGet : public Wasi<WasiFdFdstatGet> {
public:
  WasiFdFdstatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FdStatPtr);
};

class WasiFdFdstatSetFlags : public Wasi<WasiFdFdstatSetFlags> {
public:
  WasiFdFdstatSetFlags(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FsFlags);
};

class WasiFdFdstatSetRights : public Wasi<WasiFdFdstatSetRights> {
public:
  WasiFdFdstatSetRights(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t FsRightsBase, uint64_t FsRightsInheriting);
};

class WasiFdFilestatGet : public Wasi<WasiFdFilestatGet> {
public:
  WasiFdFilestatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FilestatPtr);
};

class WasiFdFilestatSetSize : public Wasi<WasiFdFilestatSetSize> {
public:
  WasiFdFilestatSetSize(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t Size);
};

class WasiFdFilestatSetTimes : public Wasi<WasiFdFilestatSetTimes> {
public:
  WasiFdFilestatSetTimes(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t ATim, uint64_t MTim, uint32_t FstFlags);
};

class WasiFdPread : public Wasi<WasiFdPread> {
public:
  WasiFdPread(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVsPtr, uint32_t IOVsLen, uint64_t Offset,
                        uint32_t NReadPtr);
};

class WasiFdPrestatGet : public Wasi<WasiFdPrestatGet> {
public:
  WasiFdPrestatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PreStatPtr);
};

class WasiFdPrestatDirName : public Wasi<WasiFdPrestatDirName> {
public:
  WasiFdPrestatDirName(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathBufPtr, uint32_t PathLen);
};

class WasiFdPwrite : public Wasi<WasiFdPwrite> {
public:
  WasiFdPwrite(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSLen, uint64_t Offset,
                        uint32_t NWrittenPtr);
};

class WasiFdRead : public Wasi<WasiFdRead> {
public:
  WasiFdRead(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSLen, uint32_t NReadPtr);
};

class WasiFdReadDir : public Wasi<WasiFdReadDir> {
public:
  WasiFdReadDir(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t BufPtr, uint32_t BufLen, uint64_t Cookie,
                        uint32_t BufUsedSize);
};

class WasiFdRenumber : public Wasi<WasiFdRenumber> {
public:
  WasiFdRenumber(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        int32_t ToFd);
};

class WasiFdSeek : public Wasi<WasiFdSeek> {
public:
  WasiFdSeek(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<int32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       int64_t Offset, uint32_t Whence, uint32_t NewOffsetPtr);
};

class WasiFdSync : public Wasi<WasiFdSync> {
public:
  WasiFdSync(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd);
};

class WasiFdTell : public Wasi<WasiFdTell> {
public:
  WasiFdTell(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t OffsetPtr);
};

class WasiFdWrite : public Wasi<WasiFdWrite> {
public:
  WasiFdWrite(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSLen,
                        uint32_t NWrittenPtr);
};

class WasiPathCreateDirectory : public Wasi<WasiPathCreateDirectory> {
public:
  WasiPathCreateDirectory(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPathFilestatGet : public Wasi<WasiPathFilestatGet> {
public:
  WasiPathFilestatGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t Flags, uint32_t PathPtr, uint32_t PathLen,
                        uint32_t FilestatPtr);
};

class WasiPathFilestatSetTimes : public Wasi<WasiPathFilestatSetTimes> {
public:
  WasiPathFilestatSetTimes(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t Flags, uint32_t PathPtr, uint32_t PathLen,
                        uint32_t ATim, uint32_t MTim, uint32_t FstFlags);
};

class WasiPathLink : public Wasi<WasiPathLink> {
public:
  WasiPathLink(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t OldFd, uint32_t OldFlags, uint32_t OldPathPtr,
                        uint32_t OldPathLen, int32_t NewFd, uint32_t NewPathPtr,
                        uint32_t NewPathLen);
};

class WasiPathOpen : public Wasi<WasiPathOpen> {
public:
  WasiPathOpen(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
                        uint32_t PathLen, uint32_t OFlags,
                        uint64_t FsRightsBase, uint64_t FsRightsInheriting,
                        uint32_t FsFlags, uint32_t FdPtr);
};

class WasiPathReadLink : public Wasi<WasiPathReadLink> {
public:
  WasiPathReadLink(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen, uint32_t BufPtr,
                        uint32_t BufLen);
};

class WasiPathRemoveDirectory : public Wasi<WasiPathRemoveDirectory> {
public:
  WasiPathRemoveDirectory(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPathRename : public Wasi<WasiPathRename> {
public:
  WasiPathRename(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t OldPathPtr, uint32_t OldPathLen, int32_t NewFd,
                        uint32_t NewPathPtr, uint32_t NewPathLen);
};

class WasiPathSymlink : public Wasi<WasiPathSymlink> {
public:
  WasiPathSymlink(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t OldPathPtr, uint32_t OldPathLen, int32_t Fd,
                        uint32_t NewPathPtr, uint32_t NewPathLen);
};

class WasiPathUnlinkFile : public Wasi<WasiPathUnlinkFile> {
public:
  WasiPathUnlinkFile(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPollOneoff : public Wasi<WasiPollOneoff> {
public:
  WasiPollOneoff(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t InPtr, uint32_t OutPtr,
                        uint32_t NSubscriptions, uint32_t NEventsPtr);
};

class WasiProcExit : public Wasi<WasiProcExit> {
public:
  WasiProcExit(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    uint32_t Status);
};

class WasiProcRaise : public Wasi<WasiProcRaise> {
public:
  WasiProcRaise(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t Signal);
};

class WasiRandomGet : public Wasi<WasiRandomGet> {
public:
  WasiRandomGet(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t BufPtr, uint32_t BufLen);
};

class WasiSchedYield : public Wasi<WasiSchedYield> {
public:
  WasiSchedYield(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class WasiSockRecv : public Wasi<WasiSockRecv> {
public:
  WasiSockRecv(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t RiDataPtr, uint32_t RiDataLen,
                        uint32_t RiFlags, uint32_t RoDataLenPtr,
                        uint32_t RoFlagsPtr);
};

class WasiSockSend : public Wasi<WasiSockSend> {
public:
  WasiSockSend(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t SiDataPtr, uint32_t SiDataLen,
                        uint32_t SiFlags, uint32_t SoDataLenPtr);
};

class WasiSockShutdown : public Wasi<WasiSockShutdown> {
public:
  WasiSockShutdown(WASI::Environ &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t SdFlags);
};

} // namespace Host
} // namespace WasmEdge
