// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasibase.h"

namespace SSVM {
namespace Host {

class WasiArgsGet : public Wasi<WasiArgsGet> {
public:
  WasiArgsGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t ArgvPtr, uint32_t ArgvBufPtr);
};

class WasiArgsSizesGet : public Wasi<WasiArgsSizesGet> {
public:
  WasiArgsSizesGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

class WasiEnvironGet : public Wasi<WasiEnvironGet> {
public:
  WasiEnvironGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t EnvPtr, uint32_t EnvBufPtr);
};

class WasiEnvironSizesGet : public Wasi<WasiEnvironSizesGet> {
public:
  WasiEnvironSizesGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               uint32_t EnvCntPtr, uint32_t EnvBufSizePtr);
};

class WasiFdClose : public Wasi<WasiFdClose> {
public:
  WasiFdClose(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd);
};

class WasiFdFdstatGet : public Wasi<WasiFdFdstatGet> {
public:
  WasiFdFdstatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, uint32_t FdStatPtr);
};

class WasiFdFdstatSetFlags : public Wasi<WasiFdFdstatSetFlags> {
public:
  WasiFdFdstatSetFlags(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, uint32_t FsFlags);
};

class WasiFdPrestatDirName : public Wasi<WasiFdPrestatDirName> {
public:
  WasiFdPrestatDirName(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, uint32_t PathBufPtr, uint32_t PathLen);
};

class WasiFdPrestatGet : public Wasi<WasiFdPrestatGet> {
public:
  WasiFdPrestatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, uint32_t PreStatPtr);
};

class WasiFdRead : public Wasi<WasiFdRead> {
public:
  WasiFdRead(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, uint32_t IOVSPtr, uint32_t IOVSCnt,
               uint32_t NReadPtr);
};

class WasiFdSeek : public Wasi<WasiFdSeek> {
public:
  WasiFdSeek(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, int32_t Offset, uint32_t Whence,
               uint32_t NewOffsetPtr);
};

class WasiFdWrite : public Wasi<WasiFdWrite> {
public:
  WasiFdWrite(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t Fd, uint32_t IOVSPtr, uint32_t IOVSCnt,
               uint32_t NWrittenPtr);
};

class WasiPathOpen : public Wasi<WasiPathOpen> {
public:
  WasiPathOpen(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, uint32_t &ErrNo,
               int32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
               uint32_t PathLen, uint32_t OFlags, uint64_t FsRightsBase,
               uint64_t FsRightsInheriting, uint32_t FsFlags, uint32_t FdPtr);
};

class WasiProcExit : public Wasi<WasiProcExit> {
public:
  WasiProcExit(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  ErrCode body(Runtime::Instance::MemoryInstance &MemInst, int32_t Status);
};

} // namespace Host
} // namespace SSVM