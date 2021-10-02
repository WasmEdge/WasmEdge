// SPDX-License-Identifier: Apache-2.0
#include "common/defines.h"
#include "gtest/gtest.h"

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS

#include "host/wasi/wasibase.h"
#include "host/wasi/wasifunc.h"
#include <string>
#include <vector>
using namespace std::literals;

namespace {

void writeDummyMemoryContent(
    WasmEdge::Runtime::Instance::MemoryInstance &MemInst) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(0), 64, UINT8_C(0xa5));
}

void writeString(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                 std::string_view String, uint32_t Ptr) noexcept {
  std::copy(String.begin(), String.end(), MemInst.getPointer<uint8_t *>(Ptr));
}

__wasi_errno_t convertErrno(int SysErrno) noexcept {
  switch (SysErrno) {
  case 0:
    return __WASI_ERRNO_SUCCESS;
  case E2BIG:
    return __WASI_ERRNO_2BIG;
  case EACCES:
    return __WASI_ERRNO_ACCES;
  case EADDRINUSE:
    return __WASI_ERRNO_ADDRINUSE;
  case EADDRNOTAVAIL:
    return __WASI_ERRNO_ADDRNOTAVAIL;
  case EAFNOSUPPORT:
    return __WASI_ERRNO_AFNOSUPPORT;
  case EAGAIN:
    return __WASI_ERRNO_AGAIN;
  case EALREADY:
    return __WASI_ERRNO_ALREADY;
  case EBADF:
    return __WASI_ERRNO_BADF;
  case EBADMSG:
    return __WASI_ERRNO_BADMSG;
  case EBUSY:
    return __WASI_ERRNO_BUSY;
  case ECANCELED:
    return __WASI_ERRNO_CANCELED;
  case ECHILD:
    return __WASI_ERRNO_CHILD;
  case ECONNABORTED:
    return __WASI_ERRNO_CONNABORTED;
  case ECONNREFUSED:
    return __WASI_ERRNO_CONNREFUSED;
  case ECONNRESET:
    return __WASI_ERRNO_CONNRESET;
  case EDEADLK:
    return __WASI_ERRNO_DEADLK;
  case EDESTADDRREQ:
    return __WASI_ERRNO_DESTADDRREQ;
  case EDOM:
    return __WASI_ERRNO_DOM;
  case EDQUOT:
    return __WASI_ERRNO_DQUOT;
  case EEXIST:
    return __WASI_ERRNO_EXIST;
  case EFAULT:
    return __WASI_ERRNO_FAULT;
  case EFBIG:
    return __WASI_ERRNO_FBIG;
  case EHOSTUNREACH:
    return __WASI_ERRNO_HOSTUNREACH;
  case EIDRM:
    return __WASI_ERRNO_IDRM;
  case EILSEQ:
    return __WASI_ERRNO_ILSEQ;
  case EINPROGRESS:
    return __WASI_ERRNO_INPROGRESS;
  case EINTR:
    return __WASI_ERRNO_INTR;
  case EINVAL:
    return __WASI_ERRNO_INVAL;
  case EIO:
    return __WASI_ERRNO_IO;
  case EISCONN:
    return __WASI_ERRNO_ISCONN;
  case EISDIR:
    return __WASI_ERRNO_ISDIR;
  case ELOOP:
    return __WASI_ERRNO_LOOP;
  case EMFILE:
    return __WASI_ERRNO_MFILE;
  case EMLINK:
    return __WASI_ERRNO_MLINK;
  case EMSGSIZE:
    return __WASI_ERRNO_MSGSIZE;
  case EMULTIHOP:
    return __WASI_ERRNO_MULTIHOP;
  case ENAMETOOLONG:
    return __WASI_ERRNO_NAMETOOLONG;
  case ENETDOWN:
    return __WASI_ERRNO_NETDOWN;
  case ENETRESET:
    return __WASI_ERRNO_NETRESET;
  case ENETUNREACH:
    return __WASI_ERRNO_NETUNREACH;
  case ENFILE:
    return __WASI_ERRNO_NFILE;
  case ENOBUFS:
    return __WASI_ERRNO_NOBUFS;
  case ENODEV:
    return __WASI_ERRNO_NODEV;
  case ENOENT:
    return __WASI_ERRNO_NOENT;
  case ENOEXEC:
    return __WASI_ERRNO_NOEXEC;
  case ENOLCK:
    return __WASI_ERRNO_NOLCK;
  case ENOLINK:
    return __WASI_ERRNO_NOLINK;
  case ENOMEM:
    return __WASI_ERRNO_NOMEM;
  case ENOMSG:
    return __WASI_ERRNO_NOMSG;
  case ENOPROTOOPT:
    return __WASI_ERRNO_NOPROTOOPT;
  case ENOSPC:
    return __WASI_ERRNO_NOSPC;
  case ENOSYS:
    return __WASI_ERRNO_NOSYS;
  case ENOTCONN:
    return __WASI_ERRNO_NOTCONN;
  case ENOTDIR:
    return __WASI_ERRNO_NOTDIR;
  case ENOTEMPTY:
    return __WASI_ERRNO_NOTEMPTY;
  case ENOTRECOVERABLE:
    return __WASI_ERRNO_NOTRECOVERABLE;
  case ENOTSOCK:
    return __WASI_ERRNO_NOTSOCK;
  case ENOTSUP:
    return __WASI_ERRNO_NOTSUP;
  case ENOTTY:
    return __WASI_ERRNO_NOTTY;
  case ENXIO:
    return __WASI_ERRNO_NXIO;
  case EOVERFLOW:
    return __WASI_ERRNO_OVERFLOW;
  case EOWNERDEAD:
    return __WASI_ERRNO_OWNERDEAD;
  case EPERM:
    return __WASI_ERRNO_PERM;
  case EPIPE:
    return __WASI_ERRNO_PIPE;
  case EPROTO:
    return __WASI_ERRNO_PROTO;
  case EPROTONOSUPPORT:
    return __WASI_ERRNO_PROTONOSUPPORT;
  case EPROTOTYPE:
    return __WASI_ERRNO_PROTOTYPE;
  case ERANGE:
    return __WASI_ERRNO_RANGE;
  case EROFS:
    return __WASI_ERRNO_ROFS;
  case ESPIPE:
    return __WASI_ERRNO_SPIPE;
  case ESRCH:
    return __WASI_ERRNO_SRCH;
  case ESTALE:
    return __WASI_ERRNO_STALE;
  case ETIMEDOUT:
    return __WASI_ERRNO_TIMEDOUT;
  case ETXTBSY:
    return __WASI_ERRNO_TXTBSY;
  case EXDEV:
    return __WASI_ERRNO_XDEV;
  default:
    __builtin_unreachable();
  }
}

uint64_t convertTimespec(const timespec &Timespec) noexcept {
  return Timespec.tv_sec * UINT64_C(1000000000) + Timespec.tv_nsec;
}

} // namespace

TEST(WasiTest, Args) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiArgsSizesGet WasiArgsSizesGet(Env);
  WasmEdge::Host::WasiArgsGet WasiArgsGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  // args: test\0
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(5));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));
  EXPECT_STREQ(MemInst.getPointer<const char *>(8), "test");
  Env.fini();

  // args: test\0 abc\0
  Env.init({}, "test"s, std::array{"abc"s}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(9));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(12));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(17));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0));
  EXPECT_STREQ(MemInst.getPointer<const char *>(12), "test");
  EXPECT_STREQ(MemInst.getPointer<const char *>(17), "abc");
  Env.fini();

  // args: test\0 \0
  Env.init({}, "test"s, std::array{""s}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(6));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(12));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(17));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0));
  EXPECT_STREQ(MemInst.getPointer<const char *>(12), "test");
  EXPECT_STREQ(MemInst.getPointer<const char *>(17), "");
  Env.fini();

  // invalid pointer
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();
}

TEST(WasiTest, Envs) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiEnvironSizesGet WasiEnvironSizesGet(Env);
  WasmEdge::Host::WasiEnvironGet WasiEnvironGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  // envs:
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  Env.fini();

  // envs: a=b\0
  Env.init({}, "test"s, {}, std::array{"a=b"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(4));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));
  EXPECT_STREQ(MemInst.getPointer<const char *>(8), "a=b");
  Env.fini();

  // envs: a=b\0 TEST=TEST=Test\0
  Env.init({}, "test"s, {}, std::array{"a=b"s, "TEST=TEST=TEST"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(19));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(12));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(16));
  EXPECT_STREQ(MemInst.getPointer<const char *>(12), "a=b");
  EXPECT_STREQ(MemInst.getPointer<const char *>(16), "TEST=TEST=TEST");
  Env.fini();

  // invalid pointer
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  // success on zero-size write
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  Env.fini();

  Env.init({}, "test"s, {}, std::array{"a=b"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();
}

TEST(WasiTest, ClockRes) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiClockResGet WasiClockResGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;
  timespec Timespec;

  Env.init({}, "test"s, {}, {});
  // realtime clock
  {
    int SysErrno = 0;
    if (clock_getres(CLOCK_REALTIME, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 2>{
            static_cast<uint32_t>(__WASI_CLOCKID_REALTIME), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Res = convertTimespec(Timespec);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Res);
    }
  }

  // monotonic clock
  {
    int SysErrno = 0;
    if (clock_getres(CLOCK_MONOTONIC, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 2>{
            static_cast<uint32_t>(__WASI_CLOCKID_MONOTONIC), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Res = convertTimespec(Timespec);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Res);
    }
  }

  // process cputime clock
  {
    int SysErrno = 0;
    if (clock_getres(CLOCK_PROCESS_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 2>{
            static_cast<uint32_t>(__WASI_CLOCKID_PROCESS_CPUTIME_ID),
            UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Res = convertTimespec(Timespec);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Res);
    }
  }

  // thread cputime clock
  {
    int SysErrno = 0;
    if (clock_getres(CLOCK_THREAD_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 2>{
            static_cast<uint32_t>(__WASI_CLOCKID_THREAD_CPUTIME_ID),
            UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Res = convertTimespec(Timespec);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Res);
    }
  }

  // invalid clockid
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(4), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
  }

  // invalid pointer
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 2>{
            static_cast<uint32_t>(__WASI_CLOCKID_REALTIME), UINT32_C(65536)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  }

  Env.fini();
}

TEST(WasiTest, ClockTimeGet) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiClockTimeGet WasiClockTimeGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;
  timespec Timespec;

  Env.init({}, "test"s, {}, {});

  // realtime clock
  {
    int SysErrno = 0;
    if (clock_gettime(CLOCK_REALTIME, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(
        WasiClockTimeGet.run(&MemInst,
                             std::array<WasmEdge::ValVariant, 3>{
                                 static_cast<uint32_t>(__WASI_CLOCKID_REALTIME),
                                 UINT64_C(0), UINT32_C(0)},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }

  // monotonic clock
  {
    int SysErrno = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 3>{
            static_cast<uint32_t>(__WASI_CLOCKID_MONOTONIC), UINT64_C(0),
            UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }

  // process cputime clock
  {
    int SysErrno = 0;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 3>{
            static_cast<uint32_t>(__WASI_CLOCKID_PROCESS_CPUTIME_ID),
            UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }

  // thread cputime clock
  {
    int SysErrno = 0;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 3>{
            static_cast<uint32_t>(__WASI_CLOCKID_THREAD_CPUTIME_ID),
            UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }

  // invalid clockid
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(&MemInst,
                                     std::array<WasmEdge::ValVariant, 3>{
                                         UINT32_C(4), UINT64_C(0), UINT32_C(0)},
                                     Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
  }

  // invalid pointer
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(
        WasiClockTimeGet.run(&MemInst,
                             std::array<WasmEdge::ValVariant, 3>{
                                 static_cast<uint32_t>(__WASI_CLOCKID_REALTIME),
                                 UINT64_C(0), UINT32_C(65536)},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  }

  Env.fini();
}

TEST(WasiTest, ProcExit) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiProcExit WasiProcExit(Env);

  Env.init({}, "test"s, {}, {});
  EXPECT_FALSE(WasiProcExit.run(
      &MemInst, std::array<WasmEdge::ValVariant, 1>{UINT32_C(0)}, {}));
  EXPECT_EQ(Env.getExitCode(), INT32_C(0));
  Env.fini();

  Env.init({}, "test"s, {}, {});
  EXPECT_FALSE(WasiProcExit.run(
      &MemInst, std::array<WasmEdge::ValVariant, 1>{UINT32_C(1)}, {}));
  EXPECT_EQ(Env.getExitCode(), INT32_C(1));
  Env.fini();
}

TEST(WasiTest, Random) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiRandomGet WasiRandomGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  // valid pointer, zero size
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiRandomGet.run(
      &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(0)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();

  // valid pointer, size 1
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiRandomGet.run(
        &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_TRUE(std::all_of(MemInst.getPointer<const uint8_t *>(1),
                            MemInst.getPointer<const uint8_t *>(4),
                            [](uint8_t x) { return x == UINT8_C(0xa5); }));
    Env.fini();
  }

  // valid pointer, size 8
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiRandomGet.run(
        &MemInst, std::array<WasmEdge::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint64_t *>(0),
              UINT64_C(0xa5a5a5a5a5a5a5a5));
    EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(8),
              UINT64_C(0xa5a5a5a5a5a5a5a5));
    Env.fini();
  }

  // invalid pointer, zero size
  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiRandomGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(0)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  Env.fini();

  // invalid pointer, non zero size
  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiRandomGet.run(
      &MemInst,
      std::array<WasmEdge::ValVariant, 2>{UINT32_C(65536), UINT32_C(1)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  Env.fini();
}

TEST(WasiTest, Directory) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiPathCreateDirectory WasiPathCreateDirectory(Env);
  WasmEdge::Host::WasiPathRemoveDirectory WasiPathRemoveDirectory(Env);
  WasmEdge::Host::WasiPathFilestatGet WasiPathFilestatGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  const uint32_t Fd = 3;
  uint32_t PathPtr = 65536;

  // invalid pointer, zero size
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, PathPtr, UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // invalid pointer, non zero size
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, PathPtr, UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    Env.fini();
  }

  PathPtr = 0;
  // zero size path
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    const auto Path = ""sv;
    const uint32_t PathSize = Path.size();
    writeString(MemInst, Path, PathPtr);
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // exists directory
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    const auto Path = "."sv;
    const uint32_t PathSize = Path.size();
    writeString(MemInst, Path, PathPtr);
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_EXIST);
    Env.fini();
  }

  // create directory, check type and remove normal directory
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    const auto Path = "tmp"sv;
    const uint32_t PathSize = Path.size();
    writeString(MemInst, Path, PathPtr);
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const uint32_t FilestatPtr = 8;
    EXPECT_TRUE(WasiPathFilestatGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{
            Fd, static_cast<uint32_t>(__WASI_LOOKUPFLAGS_SYMLINK_FOLLOW),
            PathPtr, PathSize, FilestatPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    const auto &Filestat =
        *MemInst.getPointer<const __wasi_filestat_t *>(FilestatPtr);
    EXPECT_EQ(Filestat.filetype, __WASI_FILETYPE_DIRECTORY);

    EXPECT_TRUE(WasiPathRemoveDirectory.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

TEST(WasiTest, SymbolicLink) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiPathSymlink WasiPathSymlink(Env);
  WasmEdge::Host::WasiPathUnlinkFile WasiPathUnlinkFile(Env);
  WasmEdge::Host::WasiPathFilestatGet WasiPathFilestatGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  const uint32_t Fd = 3;
  uint32_t OldPathPtr = 65536;
  uint32_t NewPathPtr = 65552;

  // invalid pointer, zero size
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathSymlink.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{OldPathPtr, UINT32_C(0), Fd,
                                            NewPathPtr, UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    Env.fini();
  }

  // invalid pointer, non zero size
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathSymlink.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{OldPathPtr, UINT32_C(0), Fd,
                                            NewPathPtr, UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    Env.fini();
  }

  OldPathPtr = 0;
  NewPathPtr = 16;
  // zero size path
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    const auto OldPath = ""sv;
    const auto NewPath = ""sv;
    const uint32_t OldPathSize = OldPath.size();
    const uint32_t NewPathSize = NewPath.size();
    writeString(MemInst, OldPath, OldPathPtr);
    writeString(MemInst, NewPath, NewPathPtr);
    EXPECT_TRUE(WasiPathSymlink.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{OldPathPtr, OldPathSize, Fd,
                                            NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // exists file
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    const auto OldPath = "."sv;
    const auto NewPath = "."sv;
    const uint32_t OldPathSize = OldPath.size();
    const uint32_t NewPathSize = NewPath.size();
    writeString(MemInst, OldPath, OldPathPtr);
    writeString(MemInst, NewPath, NewPathPtr);
    EXPECT_TRUE(WasiPathSymlink.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{OldPathPtr, OldPathSize, Fd,
                                            NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_EXIST);
    Env.fini();
  }

  // create symbolic link, check type and remove normal symbolic link
  {
    Env.init(std::array{"/:."s}, "test"s, {}, {});
    const auto OldPath = "."sv;
    const auto NewPath = "tmp"sv;
    const uint32_t OldPathSize = OldPath.size();
    const uint32_t NewPathSize = NewPath.size();
    writeString(MemInst, OldPath, OldPathPtr);
    writeString(MemInst, NewPath, NewPathPtr);
    EXPECT_TRUE(WasiPathSymlink.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{OldPathPtr, OldPathSize, Fd,
                                            NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const uint32_t FilestatPtr = 32;
    const auto &Filestat =
        *MemInst.getPointer<const __wasi_filestat_t *>(FilestatPtr);

    EXPECT_TRUE(WasiPathFilestatGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{
            Fd, static_cast<uint32_t>(0), NewPathPtr, NewPathSize, FilestatPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(Filestat.filetype, __WASI_FILETYPE_SYMBOLIC_LINK);

    EXPECT_TRUE(WasiPathFilestatGet.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 5>{
            Fd, static_cast<uint32_t>(__WASI_LOOKUPFLAGS_SYMLINK_FOLLOW),
            NewPathPtr, NewPathSize, FilestatPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(Filestat.filetype, __WASI_FILETYPE_DIRECTORY);

    EXPECT_TRUE(WasiPathUnlinkFile.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 3>{Fd, NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

#endif

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
