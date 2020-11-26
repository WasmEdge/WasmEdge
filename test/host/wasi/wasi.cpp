// SPDX-License-Identifier: Apache-2.0
#include "host/wasi/wasibase.h"
#include "host/wasi/wasifunc.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
using namespace std::literals;

namespace {
void writeDummyMemoryContent(
    SSVM::Runtime::Instance::MemoryInstance &MemInst) noexcept {
  std::fill_n(MemInst.getPointer<uint8_t *>(0), 64, UINT8_C(0xa5));
}
__wasi_errno_t convertErrno(int SysErrno) {
  switch (SysErrno) {
  case 0:
    return __WASI_ESUCCESS;
  case E2BIG:
    return __WASI_E2BIG;
  case EACCES:
    return __WASI_EACCES;
  case EADDRINUSE:
    return __WASI_EADDRINUSE;
  case EADDRNOTAVAIL:
    return __WASI_EADDRNOTAVAIL;
  case EAFNOSUPPORT:
    return __WASI_EAFNOSUPPORT;
  case EAGAIN:
    return __WASI_EAGAIN;
  case EALREADY:
    return __WASI_EALREADY;
  case EBADF:
    return __WASI_EBADF;
  case EBADMSG:
    return __WASI_EBADMSG;
  case EBUSY:
    return __WASI_EBUSY;
  case ECANCELED:
    return __WASI_ECANCELED;
  case ECHILD:
    return __WASI_ECHILD;
  case ECONNABORTED:
    return __WASI_ECONNABORTED;
  case ECONNREFUSED:
    return __WASI_ECONNREFUSED;
  case ECONNRESET:
    return __WASI_ECONNRESET;
  case EDEADLK:
    return __WASI_EDEADLK;
  case EDESTADDRREQ:
    return __WASI_EDESTADDRREQ;
  case EDOM:
    return __WASI_EDOM;
  case EDQUOT:
    return __WASI_EDQUOT;
  case EEXIST:
    return __WASI_EEXIST;
  case EFAULT:
    return __WASI_EFAULT;
  case EFBIG:
    return __WASI_EFBIG;
  case EHOSTUNREACH:
    return __WASI_EHOSTUNREACH;
  case EIDRM:
    return __WASI_EIDRM;
  case EILSEQ:
    return __WASI_EILSEQ;
  case EINPROGRESS:
    return __WASI_EINPROGRESS;
  case EINTR:
    return __WASI_EINTR;
  case EINVAL:
    return __WASI_EINVAL;
  case EIO:
    return __WASI_EIO;
  case EISCONN:
    return __WASI_EISCONN;
  case EISDIR:
    return __WASI_EISDIR;
  case ELOOP:
    return __WASI_ELOOP;
  case EMFILE:
    return __WASI_EMFILE;
  case EMLINK:
    return __WASI_EMLINK;
  case EMSGSIZE:
    return __WASI_EMSGSIZE;
  case EMULTIHOP:
    return __WASI_EMULTIHOP;
  case ENAMETOOLONG:
    return __WASI_ENAMETOOLONG;
  case ENETDOWN:
    return __WASI_ENETDOWN;
  case ENETRESET:
    return __WASI_ENETRESET;
  case ENETUNREACH:
    return __WASI_ENETUNREACH;
  case ENFILE:
    return __WASI_ENFILE;
  case ENOBUFS:
    return __WASI_ENOBUFS;
  case ENODEV:
    return __WASI_ENODEV;
  case ENOENT:
    return __WASI_ENOENT;
  case ENOEXEC:
    return __WASI_ENOEXEC;
  case ENOLCK:
    return __WASI_ENOLCK;
  case ENOLINK:
    return __WASI_ENOLINK;
  case ENOMEM:
    return __WASI_ENOMEM;
  case ENOMSG:
    return __WASI_ENOMSG;
  case ENOPROTOOPT:
    return __WASI_ENOPROTOOPT;
  case ENOSPC:
    return __WASI_ENOSPC;
  case ENOSYS:
    return __WASI_ENOSYS;
  case ENOTCONN:
    return __WASI_ENOTCONN;
  case ENOTDIR:
    return __WASI_ENOTDIR;
  case ENOTEMPTY:
    return __WASI_ENOTEMPTY;
  case ENOTRECOVERABLE:
    return __WASI_ENOTRECOVERABLE;
  case ENOTSOCK:
    return __WASI_ENOTSOCK;
  case ENOTSUP:
    return __WASI_ENOTSUP;
  case ENOTTY:
    return __WASI_ENOTTY;
  case ENXIO:
    return __WASI_ENXIO;
  case EOVERFLOW:
    return __WASI_EOVERFLOW;
  case EOWNERDEAD:
    return __WASI_EOWNERDEAD;
  case EPERM:
    return __WASI_EPERM;
  case EPIPE:
    return __WASI_EPIPE;
  case EPROTO:
    return __WASI_EPROTO;
  case EPROTONOSUPPORT:
    return __WASI_EPROTONOSUPPORT;
  case EPROTOTYPE:
    return __WASI_EPROTOTYPE;
  case ERANGE:
    return __WASI_ERANGE;
  case EROFS:
    return __WASI_EROFS;
  case ESPIPE:
    return __WASI_ESPIPE;
  case ESRCH:
    return __WASI_ESRCH;
  case ESTALE:
    return __WASI_ESTALE;
  case ETIMEDOUT:
    return __WASI_ETIMEDOUT;
  case ETXTBSY:
    return __WASI_ETXTBSY;
  case EXDEV:
    return __WASI_EXDEV;
  default:
    __builtin_unreachable();
  }
}

uint64_t convertTimespec(const timespec &Timespec) noexcept {
  return Timespec.tv_sec * UINT64_C(1000000000) + Timespec.tv_nsec;
}

} // namespace

TEST(WasiTest, Args) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  SSVM::Host::WasiArgsSizesGet WasiArgsSizesGet(Env);
  SSVM::Host::WasiArgsGet WasiArgsGet(Env);
  std::array<SSVM::ValVariant, 1> Errno;

  // args: test\0
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(5));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));
  EXPECT_STREQ(MemInst.getPointer<const char *>(8), "test");
  Env.fini();

  // args: test\0 abc\0
  Env.init({}, "test"s, std::array{"abc"s}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(9));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
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
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(6));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
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
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiArgsSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiArgsGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();
}

TEST(WasiTest, Envs) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  SSVM::Host::WasiEnvironSizesGet WasiEnvironSizesGet(Env);
  SSVM::Host::WasiEnvironGet WasiEnvironGet(Env);
  std::array<SSVM::ValVariant, 1> Errno;

  // envs:
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  Env.fini();

  // envs: a=b\0
  Env.init({}, "test"s, {}, std::array{"a=b"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(4));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));
  EXPECT_STREQ(MemInst.getPointer<const char *>(8), "a=b");
  Env.fini();

  // envs: a=b\0 TEST=TEST=Test\0
  Env.init({}, "test"s, {}, std::array{"a=b"s, "TEST=TEST=TEST"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(19));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(12)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(12));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(16));
  EXPECT_STREQ(MemInst.getPointer<const char *>(12), "a=b");
  EXPECT_STREQ(MemInst.getPointer<const char *>(16), "TEST=TEST=TEST");
  Env.fini();

  // invalid pointer
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  // success on zero-size write
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  Env.fini();

  Env.init({}, "test"s, {}, std::array{"a=b"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();
}

TEST(WasiTest, ClockRes) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  SSVM::Host::WasiClockResGet WasiClockResGet(Env);
  std::array<SSVM::ValVariant, 1> Errno;
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
        std::array<SSVM::ValVariant, 2>{__WASI_CLOCK_REALTIME, UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
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
        std::array<SSVM::ValVariant, 2>{__WASI_CLOCK_MONOTONIC, UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
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
    EXPECT_TRUE(
        WasiClockResGet.run(&MemInst,
                            std::array<SSVM::ValVariant, 2>{
                                __WASI_CLOCK_PROCESS_CPUTIME_ID, UINT32_C(0)},
                            Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
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
    EXPECT_TRUE(
        WasiClockResGet.run(&MemInst,
                            std::array<SSVM::ValVariant, 2>{
                                __WASI_CLOCK_THREAD_CPUTIME_ID, UINT32_C(0)},
                            Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Res = convertTimespec(Timespec);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Res);
    }
  }

  // invalid clockid
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(4), UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EINVAL);
  }

  // invalid pointer
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        &MemInst,
        std::array<SSVM::ValVariant, 2>{__WASI_CLOCK_REALTIME, UINT32_C(65536)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  }

  Env.fini();
}

TEST(WasiTest, ClockTimeGet) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  SSVM::Host::WasiClockTimeGet WasiClockTimeGet(Env);
  std::array<SSVM::ValVariant, 1> Errno;
  timespec Timespec;

  Env.init({}, "test"s, {}, {});

  // realtime clock
  {
    int SysErrno = 0;
    if (clock_gettime(CLOCK_REALTIME, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        &MemInst,
        std::array<SSVM::ValVariant, 3>{__WASI_CLOCK_REALTIME, UINT64_C(0),
                                        UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
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
        std::array<SSVM::ValVariant, 3>{__WASI_CLOCK_MONOTONIC, UINT64_C(0),
                                        UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
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
        std::array<SSVM::ValVariant, 3>{__WASI_CLOCK_PROCESS_CPUTIME_ID,
                                        UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
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
        std::array<SSVM::ValVariant, 3>{__WASI_CLOCK_THREAD_CPUTIME_ID,
                                        UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }

  // invalid clockid
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        &MemInst,
        std::array<SSVM::ValVariant, 3>{UINT32_C(4), UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EINVAL);
  }

  // invalid pointer
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        &MemInst,
        std::array<SSVM::ValVariant, 3>{__WASI_CLOCK_REALTIME, UINT64_C(0),
                                        UINT32_C(65536)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  }

  Env.fini();
}

TEST(WasiTest, ProcExit) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  SSVM::Host::WasiProcExit WasiProcExit(Env);

  Env.init({}, "test"s, {}, {});
  EXPECT_FALSE(WasiProcExit.run(
      &MemInst, std::array<SSVM::ValVariant, 1>{UINT32_C(0)}, {}));
  EXPECT_EQ(Env.getExitCode(), INT32_C(0));
  Env.fini();

  Env.init({}, "test"s, {}, {});
  EXPECT_FALSE(WasiProcExit.run(
      &MemInst, std::array<SSVM::ValVariant, 1>{UINT32_C(1)}, {}));
  EXPECT_EQ(Env.getExitCode(), INT32_C(1));
  Env.fini();
}

TEST(WasiTest, Random) {
  SSVM::Host::WasiEnvironment Env;
  SSVM::Runtime::Instance::MemoryInstance MemInst(SSVM::AST::Limit(1));

  SSVM::Host::WasiRandomGet WasiRandomGet(Env);
  std::array<SSVM::ValVariant, 1> Errno;

  // valid pointer, zero size
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiRandomGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(0)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();

  // valid pointer, size 1
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiRandomGet.run(
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(1)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
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
        &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(0), UINT32_C(8)},
        Errno));
    EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
    EXPECT_NE(*MemInst.getPointer<const uint64_t *>(0),
              UINT64_C(0xa5a5a5a5a5a5a5a5));
    EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(8),
              UINT64_C(0xa5a5a5a5a5a5a5a5));
    Env.fini();
  }

  // invalid pointer, zero size
  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiRandomGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(0)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_ESUCCESS);
  Env.fini();

  // invalid pointer, non zero size
  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiRandomGet.run(
      &MemInst, std::array<SSVM::ValVariant, 2>{UINT32_C(65536), UINT32_C(1)},
      Errno));
  EXPECT_EQ(SSVM::retrieveValue<int32_t>(Errno[0]), __WASI_EFAULT);
  Env.fini();
}

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
