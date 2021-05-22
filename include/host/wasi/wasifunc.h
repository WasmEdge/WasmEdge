// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "wasibase.h"

/// Definitions on MacOS
#ifdef __APPLE__
/// TODO: Implement functions bellow on MacOS
/// Posix functions
#define posix_fallocate(...) ((uint32_t)0)
#define posix_fadvise(...) ((uint32_t)0)
#define fdatasync(...) ((uint32_t)0)
#define POSIX_FADV_NORMAL 0     /* No further special treatment. */
#define POSIX_FADV_RANDOM 1     /* Expect random page references. */
#define POSIX_FADV_SEQUENTIAL 2 /* Expect sequential page references. */
#define POSIX_FADV_WILLNEED 3   /* Will need these pages. */
#define POSIX_FADV_DONTNEED 4   /* Don't need these pages. */
#define POSIX_FADV_NOREUSE 5    /* Data will be accessed once. */

/// Epoll structures
typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event {
  uint32_t events;   /* Epoll events */
  epoll_data_t data; /* User data variable */
};

/// Epoll functions
#define EPOLLIN 0x001
#define EPOLLPRI 0x002
#define EPOLLOUT 0x004
#define EPOLLRDNORM 0x040
#define EPOLLRDBAND 0x080
#define EPOLLWRNORM 0x100
#define EPOLLWRBAND 0x200
#define EPOLLMSG 0x400
#define EPOLLERR 0x008
#define EPOLLHUP 0x010
#define EPOLLRDHUP 0x2000
#define EPOLLONESHOT (1 << 30)
#define EPOLLET (1 << 31)
#define epoll_create1(...) ((uint32_t)0)
#define epoll_ctl(...) ((uint32_t)0)
#define epoll_pwait(...) ((uint32_t)0)

/// Signalfd structures
struct signalfd_siginfo {
  uint32_t ssi_signo;    /* Signal number */
  int32_t ssi_errno;     /* Error number (unused) */
  int32_t ssi_code;      /* Signal code */
  uint32_t ssi_pid;      /* PID of sender */
  uint32_t ssi_uid;      /* Real UID of sender */
  int32_t ssi_fd;        /* File descriptor (SIGIO) */
  uint32_t ssi_tid;      /* Kernel timer ID (POSIX timers) */
  uint32_t ssi_band;     /* Band event (SIGIO) */
  uint32_t ssi_overrun;  /* POSIX timer overrun count */
  uint32_t ssi_trapno;   /* Trap number that caused signal */
  int32_t ssi_status;    /* Exit status or signal (SIGCHLD) */
  int32_t ssi_int;       /* Integer sent by sigqueue(3) */
  uint64_t ssi_ptr;      /* Pointer sent by sigqueue(3) */
  uint64_t ssi_utime;    /* User CPU time consumed (SIGCHLD) */
  uint64_t ssi_stime;    /* System CPU time consumed
                            (SIGCHLD) */
  uint64_t ssi_addr;     /* Address that generated signal
                            (for hardware-generated signals) */
  uint16_t ssi_addr_lsb; /* Least significant bit of address
                            (SIGBUS; since Linux 2.6.37) */
  uint8_t pad[46];       /* Pad size to 128 bytes (allow for
                            additional fields in the future) */
};

/// Signalfd functions
#define signalfd(...) ((uint32_t)0)
#define SIGPOLL SIGIO
#define SIGPWR 30
#define SIGRTMIN 34

/// Preadv
#define preadv(...) ((uint32_t)0)
#define pwritev(...) ((uint32_t)0)
#define O_RSYNC 0x0800

/// Timer
struct itimerspec {
  struct timespec it_interval; /* timer period */
  struct timespec it_value;    /* timer expiration */
};
#define TIMER_ABSTIME 0x01
#define timer_t int
#define timer_create(...) ((uint32_t)0)
#define timer_settime(...) ((uint32_t)0)
#define timer_delete(...) ((uint32_t)0)
#endif

namespace WasmEdge {
namespace Host {

class WasiArgsGet : public Wasi<WasiArgsGet> {
public:
  WasiArgsGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ArgvPtr, uint32_t ArgvBufPtr);
};

class WasiArgsSizesGet : public Wasi<WasiArgsSizesGet> {
public:
  WasiArgsSizesGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ArgcPtr, uint32_t ArgvBufSizePtr);
};

class WasiEnvironGet : public Wasi<WasiEnvironGet> {
public:
  WasiEnvironGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t EnvPtr, uint32_t EnvBufPtr);
};

class WasiEnvironSizesGet : public Wasi<WasiEnvironSizesGet> {
public:
  WasiEnvironSizesGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t EnvCntPtr, uint32_t EnvBufSizePtr);
};

class WasiClockResGet : public Wasi<WasiClockResGet> {
public:
  WasiClockResGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ClockId, uint32_t ResolutionPtr);
};

class WasiClockTimeGet : public Wasi<WasiClockTimeGet> {
public:
  WasiClockTimeGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t ClockId, uint64_t Precision, uint32_t TimePtr);
};

class WasiFdAdvise : public Wasi<WasiFdAdvise> {
public:
  WasiFdAdvise(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t Offset, uint64_t Len, uint32_t Advice);
};

class WasiFdAllocate : public Wasi<WasiFdAllocate> {
public:
  WasiFdAllocate(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t Offset, uint64_t Len);
};

class WasiFdClose : public Wasi<WasiFdClose> {
public:
  WasiFdClose(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd);
};

class WasiFdDatasync : public Wasi<WasiFdDatasync> {
public:
  WasiFdDatasync(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd);
};

class WasiFdFdstatGet : public Wasi<WasiFdFdstatGet> {
public:
  WasiFdFdstatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FdStatPtr);
};

class WasiFdFdstatSetFlags : public Wasi<WasiFdFdstatSetFlags> {
public:
  WasiFdFdstatSetFlags(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FsFlags);
};

class WasiFdFdstatSetRights : public Wasi<WasiFdFdstatSetRights> {
public:
  WasiFdFdstatSetRights(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t FsRightsBase, uint64_t FsRightsInheriting);
};

class WasiFdFilestatGet : public Wasi<WasiFdFilestatGet> {
public:
  WasiFdFilestatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t FilestatPtr);
};

class WasiFdFilestatSetSize : public Wasi<WasiFdFilestatSetSize> {
public:
  WasiFdFilestatSetSize(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t FileSize);
};

class WasiFdFilestatSetTimes : public Wasi<WasiFdFilestatSetTimes> {
public:
  WasiFdFilestatSetTimes(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint64_t ATim, uint64_t MTim, uint32_t FstFlags);
};

class WasiFdPread : public Wasi<WasiFdPread> {
public:
  WasiFdPread(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, int32_t IOVSLen, uint64_t Offset,
                        uint32_t NReadPtr);
};

class WasiFdPrestatGet : public Wasi<WasiFdPrestatGet> {
public:
  WasiFdPrestatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PreStatPtr);
};

class WasiFdPrestatDirName : public Wasi<WasiFdPrestatDirName> {
public:
  WasiFdPrestatDirName(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathBufPtr, uint32_t PathLen);
};

class WasiFdPwrite : public Wasi<WasiFdPwrite> {
public:
  WasiFdPwrite(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, int32_t IOVSLen, uint64_t Offset,
                        uint32_t NWrittenPtr);
};

class WasiFdRead : public Wasi<WasiFdRead> {
public:
  WasiFdRead(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, int32_t IOVSLen, uint32_t NReadPtr);
};

class WasiFdReadDir : public Wasi<WasiFdReadDir> {
public:
  WasiFdReadDir(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t BufPtr, uint32_t BufLen, uint64_t Cookie,
                        uint32_t BufUsedSizePtr);
};

class WasiFdRenumber : public Wasi<WasiFdRenumber> {
public:
  WasiFdRenumber(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        int32_t ToFd);
};

class WasiFdSeek : public Wasi<WasiFdSeek> {
public:
  WasiFdSeek(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<int32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                       int64_t Offset, uint32_t Whence, uint32_t NewOffsetPtr);
};

class WasiFdSync : public Wasi<WasiFdSync> {
public:
  WasiFdSync(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd);
};

class WasiFdTell : public Wasi<WasiFdTell> {
public:
  WasiFdTell(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        int32_t OffsetPtr);
};

class WasiFdWrite : public Wasi<WasiFdWrite> {
public:
  WasiFdWrite(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t IOVSPtr, int32_t IOVSLen,
                        uint32_t NWrittenPtr);
};

class WasiPathCreateDirectory : public Wasi<WasiPathCreateDirectory> {
public:
  WasiPathCreateDirectory(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPathFilestatGet : public Wasi<WasiPathFilestatGet> {
public:
  WasiPathFilestatGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t Flags, uint32_t PathPtr, uint32_t PathLen,
                        uint32_t FilestatPtr);
};

class WasiPathFilestatSetTimes : public Wasi<WasiPathFilestatSetTimes> {
public:
  WasiPathFilestatSetTimes(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t Flags, uint32_t PathPtr, uint32_t PathLen,
                        uint32_t ATim, uint32_t MTim, uint32_t FstFlags);
};

class WasiPathLink : public Wasi<WasiPathLink> {
public:
  WasiPathLink(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t OldFd, uint32_t OldFlags, uint32_t OldPathPtr,
                        uint32_t OldPathLen, int32_t NewFd, uint32_t NewPathPtr,
                        uint32_t NewPathLen);
};

class WasiPathOpen : public Wasi<WasiPathOpen> {
public:
  WasiPathOpen(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t DirFd, uint32_t DirFlags, uint32_t PathPtr,
                        uint32_t PathLen, uint32_t OFlags,
                        uint64_t FsRightsBase, uint64_t FsRightsInheriting,
                        uint32_t FsFlags, uint32_t FdPtr);
};

class WasiPathReadLink : public Wasi<WasiPathReadLink> {
public:
  WasiPathReadLink(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen, uint32_t BufPtr,
                        uint32_t BufLen, uint32_t BufUsedPtr);
};

class WasiPathRemoveDirectory : public Wasi<WasiPathRemoveDirectory> {
public:
  WasiPathRemoveDirectory(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPathRename : public Wasi<WasiPathRename> {
public:
  WasiPathRename(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t OldFd, uint32_t OldPathPtr, uint32_t OldPathLen,
                        int32_t NewFd, uint32_t NewPathPtr,
                        uint32_t NewPathLen);
};

class WasiPathSymlink : public Wasi<WasiPathSymlink> {
public:
  WasiPathSymlink(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t OldPathPtr, uint32_t OldPathLen, int32_t Fd,
                        uint32_t NewPathPtr, uint32_t NewPathLen);
};

class WasiPathUnlinkFile : public Wasi<WasiPathUnlinkFile> {
public:
  WasiPathUnlinkFile(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t PathPtr, uint32_t PathLen);
};

class WasiPollOneoff : public Wasi<WasiPollOneoff> {
public:
  WasiPollOneoff(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t InPtr, uint32_t OutPtr,
                        uint32_t NSubscriptions, uint32_t NEventsPtr);
};

class WasiProcExit : public Wasi<WasiProcExit> {
public:
  WasiProcExit(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Status);
};

class WasiProcRaise : public Wasi<WasiProcRaise> {
public:
  WasiProcRaise(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        int32_t Signal);
};

class WasiRandomGet : public Wasi<WasiRandomGet> {
public:
  WasiRandomGet(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        uint32_t BufPtr, uint32_t BufLen);
};

class WasiSchedYield : public Wasi<WasiSchedYield> {
public:
  WasiSchedYield(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst);
};

class WasiSockRecv : public Wasi<WasiSockRecv> {
public:
  WasiSockRecv(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t RiDataPtr, int32_t RiDataLen, uint32_t RiFlags,
                        uint32_t RoDataLenPtr, uint32_t RoFlagsPtr);
};

class WasiSockSend : public Wasi<WasiSockSend> {
public:
  WasiSockSend(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t SiDataPtr, int32_t SiDataLen, uint32_t SiFlags,
                        uint32_t SoDataLenPtr);
};

class WasiSockShutdown : public Wasi<WasiSockShutdown> {
public:
  WasiSockShutdown(WasiEnvironment &HostEnv) : Wasi(HostEnv) {}

  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst, int32_t Fd,
                        uint32_t SdFlags);
};

} // namespace Host
} // namespace WasmEdge
