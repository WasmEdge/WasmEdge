// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasibase.h"

/// POSIX Definitions on MacOS
#ifdef __APPLE__
/// TODO: Implement functions bellow on MacOS
#define posix_fallocate(...) ((uint32_t)0)
#define posix_fadvise(...) ((uint32_t)0)
#define fdatasync(...) ((uint32_t)0)
#define POSIX_FADV_NORMAL 0     /* No further special treatment. */
#define POSIX_FADV_RANDOM 1     /* Expect random page references. */
#define POSIX_FADV_SEQUENTIAL 2 /* Expect sequential page references. */
#define POSIX_FADV_WILLNEED 3   /* Will need these pages. */
#define POSIX_FADV_DONTNEED 4   /* Don't need these pages. */
#define POSIX_FADV_NOREUSE 5    /* Data will be accessed once. */
#endif

namespace SSVM {
namespace Host {

class WasiArgsGet : public Wasi<WasiArgsGet> {
public:
  WasiArgsGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t ArgvPtr, uint32_t ArgvBufPtr);
};

class WasiArgsSizesGet : public Wasi<WasiArgsSizesGet> {
public:
  WasiArgsSizesGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

class WasiEnvironGet : public Wasi<WasiEnvironGet> {
public:
  WasiEnvironGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t EnvPtr, uint32_t EnvBufPtr);
};

class WasiEnvironSizesGet : public Wasi<WasiEnvironSizesGet> {
public:
  WasiEnvironSizesGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t EnvCntPtr, uint32_t EnvBufSizePtr);
};

class WasiClockResGet : public Wasi<WasiClockResGet> {
public:
  WasiClockResGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t ClockId, uint32_t ResolutionPtr);
};

class WasiClockTimeGet : public Wasi<WasiClockTimeGet> {
public:
  WasiClockTimeGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        uint32_t ClockId, uint64_t Precision, uint32_t TimePtr);
};

class WasiFdAdvise : public Wasi<WasiFdAdvise> {
public:
  WasiFdAdvise(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint64_t Offset, uint64_t Len, uint32_t Advice);
};

class WasiFdAllocate : public Wasi<WasiFdAllocate> {
public:
  WasiFdAllocate(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint64_t Offset, uint64_t Len);
};

class WasiFdClose : public Wasi<WasiFdClose> {
public:
  WasiFdClose(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd);
};

class WasiFdDatasync : public Wasi<WasiFdDatasync> {
public:
  WasiFdDatasync(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd);
};

class WasiFdFdstatGet : public Wasi<WasiFdFdstatGet> {
public:
  WasiFdFdstatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t FdStatPtr);
};

class WasiFdFdstatSetFlags : public Wasi<WasiFdFdstatSetFlags> {
public:
  WasiFdFdstatSetFlags(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t FsFlags);
};

class WasiFdPrestatDirName : public Wasi<WasiFdPrestatDirName> {
public:
  WasiFdPrestatDirName(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t PathBufPtr, uint32_t PathLen);
};

class WasiFdPrestatGet : public Wasi<WasiFdPrestatGet> {
public:
  WasiFdPrestatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t PreStatPtr);
};

class WasiFdRead : public Wasi<WasiFdRead> {
public:
  WasiFdRead(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSCnt, uint32_t NReadPtr);
};

class WasiFdSeek : public Wasi<WasiFdSeek> {
public:
  WasiFdSeek(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<int32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                       int64_t Offset, uint32_t Whence, uint32_t NewOffsetPtr);
};

class WasiFdWrite : public Wasi<WasiFdWrite> {
public:
  WasiFdWrite(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Fd,
                        uint32_t IOVSPtr, uint32_t IOVSCnt,
                        uint32_t NWrittenPtr);
};

class WasiPathOpen : public Wasi<WasiPathOpen> {
public:
  WasiPathOpen(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance &MemInst,
                        int32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
                        uint32_t PathLen, uint32_t OFlags,
                        uint64_t FsRightsBase, uint64_t FsRightsInheriting,
                        uint32_t FsFlags, uint32_t FdPtr);
};

class WasiProcExit : public Wasi<WasiProcExit> {
public:
  WasiProcExit(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<void> body(Runtime::Instance::MemoryInstance &MemInst, int32_t Status);
};

class WasiRandomGet : public Wasi<WasiRandomGet> {
public:
  WasiRandomGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<void> body(Runtime::Instance::MemoryInstance &MemInst, uint32_t BufPtr,
                    uint32_t BufLen);
};

} // namespace Host
} // namespace SSVM
