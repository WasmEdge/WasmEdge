// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "common/defines.h"
#include <gtest/gtest.h>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS

#include "host/wasi/wasibase.h"
#include "host/wasi/wasifunc.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

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

void writeAddress(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                  WasmEdge::Span<const uint8_t> Address, uint32_t Ptr) {
  const uint32_t BufPtr = Ptr + sizeof(__wasi_address_t);
  std::copy(Address.begin(), Address.end(),
            MemInst.getPointer<uint8_t *>(BufPtr));

  __wasi_address_t WasiAddress;
  WasiAddress.buf = BufPtr;
  WasiAddress.buf_len = Address.size();

  std::memcpy(
      MemInst.getPointer<__wasi_address_t *>(Ptr, sizeof(__wasi_address_t)),
      &WasiAddress, sizeof(__wasi_address_t));
}

void writeAddrinfo(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                   __wasi_addrinfo_t *WasiAddrinfo, uint32_t Ptr) {
  std::memcpy(
      MemInst.getPointer<__wasi_addrinfo_t *>(Ptr, sizeof(__wasi_addrinfo_t)),
      WasiAddrinfo, sizeof(__wasi_addrinfo_t));
}

void allocateAddrinfoArray(WasmEdge::Runtime::Instance::MemoryInstance &MemInst,
                           uint32_t Base, uint32_t Length,
                           uint32_t CanonnameMaxSize) {
  for (uint32_t Item = 0; Item < Length; Item++) {
    // allocate addrinfo struct
    auto *ResItemPtr = MemInst.getPointer<__wasi_addrinfo_t *>(
        Base, sizeof(__wasi_addrinfo_t));
    Base += sizeof(__wasi_addrinfo_t);

    // allocate sockaddr struct
    ResItemPtr->ai_addr = Base;
    ResItemPtr->ai_addrlen = sizeof(__wasi_sockaddr_t);
    auto *Sockaddr = MemInst.getPointer<__wasi_sockaddr_t *>(
        ResItemPtr->ai_addr, sizeof(__wasi_sockaddr_t));
    Base += ResItemPtr->ai_addrlen;
    // allocate sockaddr sa_data.
    Sockaddr->sa_data = Base;
    Sockaddr->sa_data_len = WasmEdge::Host::WASI::kSaDataLen;
    Base += Sockaddr->sa_data_len;
    // allocate ai_canonname
    ResItemPtr->ai_canonname = Base;
    ResItemPtr->ai_canonname_len = CanonnameMaxSize;
    Base += ResItemPtr->ai_canonname_len;
    if (Item != (Length - 1)) {
      ResItemPtr->ai_next = Base;
    }
  }
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
    assumingUnreachable();
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

TEST(WasiTest, PollOneoffSocket) {
  enum class ServerAction {
    None,
    Stop,
    Start,
    Send,
    Recv,
  };
  std::atomic<ServerAction> Action(ServerAction::Start);
  std::atomic_bool ActionDone(false);
  std::mutex Mutex;
  std::condition_variable ActionRequested;
  std::condition_variable ActionProcessed;
  const std::array<uint8_t, 4> Address{127, 0, 0, 1};
  const uint32_t Port = 18000;

  std::thread Server([&]() {
    WasmEdge::Host::WASI::Environ Env;
    WasmEdge::Runtime::Instance::MemoryInstance MemInst(
        WasmEdge::AST::MemoryType(1));

    WasmEdge::Host::WasiFdClose WasiFdClose(Env);
    WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
    WasmEdge::Host::WasiSockAccept WasiSockAccept(Env);
    WasmEdge::Host::WasiSockBind WasiSockBind(Env);
    WasmEdge::Host::WasiSockListen WasiSockListen(Env);
    WasmEdge::Host::WasiSockOpen WasiSockOpen(Env);
    WasmEdge::Host::WasiSockRecv WasiSockRecv(Env);
    WasmEdge::Host::WasiSockSend WasiSockSend(Env);
    WasmEdge::Host::WasiSockSetOpt WasiSockSetOpt(Env);

    std::array<WasmEdge::ValVariant, 1> Errno;
    const uint32_t FdPtr = 0;
    const uint32_t AddressPtr = 4;
    const uint32_t Backlog = 1;
    int32_t ConnectionFd = -1;

    Env.init({}, "test"s, {}, {});
    while (true) {
      {
        std::unique_lock<std::mutex> Lock(Mutex);
        ActionRequested.wait(Lock,
                             [&]() { return Action != ServerAction::None; });
      }
      switch (Action.exchange(ServerAction::None, std::memory_order_acquire)) {
      case ServerAction::None: {
        continue;
      }
      case ServerAction::Stop: {
        // close socket
        EXPECT_TRUE(WasiFdClose.run(
            &MemInst, std::array<WasmEdge::ValVariant, 1>{ConnectionFd},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        Env.fini();
        return;
      }
      case ServerAction::Start: {
        int32_t ServerFd = -1;
        // open socket
        EXPECT_TRUE(WasiSockOpen.run(
            &MemInst,
            std::array<WasmEdge::ValVariant, 3>{
                static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
                static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE(MemInst.loadValue(ServerFd, FdPtr, sizeof(ServerFd)));

        // set socket options
        const uint32_t SockOptionsPtr = 0;
        const uint32_t One = 1;
        MemInst.storeValue(One, SockOptionsPtr, sizeof(One));
        EXPECT_TRUE(WasiSockSetOpt.run(
            &MemInst,
            std::array<WasmEdge::ValVariant, 5>{
                ServerFd,
                static_cast<uint32_t>(__WASI_SOCK_OPT_LEVEL_SOL_SOCKET),
                static_cast<uint32_t>(__WASI_SOCK_OPT_SO_REUSEADDR),
                static_cast<uint32_t>(SockOptionsPtr),
                static_cast<uint32_t>(sizeof(One))},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // bind port
        writeAddress(MemInst, Address, AddressPtr);
        EXPECT_TRUE(WasiSockBind.run(
            &MemInst,
            std::array<WasmEdge::ValVariant, 3>{ServerFd, AddressPtr, Port},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        ActionDone.store(true);
        ActionProcessed.notify_one();

        // listen port
        EXPECT_TRUE(WasiSockListen.run(
            &MemInst, std::array<WasmEdge::ValVariant, 2>{ServerFd, Backlog},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // accept port
        EXPECT_TRUE(WasiSockAccept.run(
            &MemInst, std::array<WasmEdge::ValVariant, 2>{ServerFd, FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE(
            MemInst.loadValue(ConnectionFd, FdPtr, sizeof(ConnectionFd)));

        // close socket
        EXPECT_TRUE(WasiFdClose.run(
            &MemInst, std::array<WasmEdge::ValVariant, 1>{ServerFd}, Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // set nonblock flag
        EXPECT_TRUE(WasiFdFdstatSetFlags.run(
            &MemInst,
            std::array<WasmEdge::ValVariant, 2>{
                ConnectionFd, static_cast<uint32_t>(__WASI_FDFLAGS_NONBLOCK)},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        continue;
      }
      case ServerAction::Send: {
        const uint32_t IOVecSize = 1;
        const uint32_t NWrittenPtr = 0;
        const uint32_t IOVecPtr = NWrittenPtr + sizeof(__wasi_size_t);
        const uint32_t DataPtr = IOVecPtr + sizeof(__wasi_ciovec_t) * IOVecSize;
        const uint32_t SiFlags = 0;
        const auto Data = "server"sv;
        writeString(MemInst, Data, DataPtr);
        auto *IOVec = MemInst.getPointer<__wasi_ciovec_t *>(
            IOVecPtr, sizeof(__wasi_ciovec_t) * IOVecSize);
        IOVec[0].buf = DataPtr;
        IOVec[0].buf_len = Data.size();
        EXPECT_TRUE(WasiSockSend.run(
            &MemInst,
            std::array<WasmEdge::ValVariant, 5>{
                ConnectionFd, IOVecPtr, IOVecSize, SiFlags, NWrittenPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        __wasi_size_t NWritten;
        EXPECT_TRUE(MemInst.loadValue(NWritten, NWrittenPtr, sizeof(NWritten)));
        EXPECT_EQ(NWritten, Data.size());

        ActionDone.store(true);
        ActionProcessed.notify_one();
        continue;
      }
      case ServerAction::Recv: {
        // read data until buffer empty
        while (true) {
          const uint32_t IOVecSize = 1;
          const uint32_t NReadPtr = 0;
          const uint32_t RoFlagsPtr = NReadPtr + sizeof(__wasi_size_t);
          const uint32_t IOVecPtr = RoFlagsPtr + sizeof(__wasi_size_t);
          const uint32_t DataPtr =
              IOVecPtr + sizeof(__wasi_iovec_t) * IOVecSize;
          const uint32_t RiFlags = 0;
          auto *IOVec = MemInst.getPointer<__wasi_ciovec_t *>(
              IOVecPtr, sizeof(__wasi_ciovec_t) * IOVecSize);
          IOVec[0].buf = DataPtr;
          IOVec[0].buf_len = 32768;
          EXPECT_TRUE(WasiSockRecv.run(&MemInst,
                                       std::array<WasmEdge::ValVariant, 6>{
                                           ConnectionFd, IOVecPtr, IOVecSize,
                                           RiFlags, NReadPtr, RoFlagsPtr},
                                       Errno));
          if (Errno[0].get<int32_t>() != __WASI_ERRNO_SUCCESS) {
            EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AGAIN);
            break;
          }
        }

        ActionDone.store(true);
        ActionProcessed.notify_one();
        continue;
      }
      }
    }
  });

  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
  WasmEdge::Host::WasiPollOneoff WasiPollOneoff(Env);
  WasmEdge::Host::WasiSockConnect WasiSockConnect(Env);
  WasmEdge::Host::WasiSockOpen WasiSockOpen(Env);
  WasmEdge::Host::WasiSockRecv WasiSockRecv(Env);
  WasmEdge::Host::WasiSockSend WasiSockSend(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  const uint32_t FdPtr = 0;
  const uint32_t AddressPtr = 4;

  {
    Env.init({}, "test"s, {}, {});

    // open socket
    EXPECT_TRUE(WasiSockOpen.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 3>{
            static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
            static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    int32_t Fd;
    EXPECT_TRUE(MemInst.loadValue(Fd, FdPtr, sizeof(Fd)));

    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // connect server
    writeAddress(MemInst, Address, AddressPtr);
    EXPECT_TRUE(WasiSockConnect.run(
        &MemInst, std::array<WasmEdge::ValVariant, 3>{Fd, AddressPtr, Port},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    auto PollReadTimeout = [&]() {
      const uint32_t Count = 2;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_READ;
      Subscriptions[0].u.u.fd_read.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[1].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[1].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[1].u.u.clock.precision = 1;
      Subscriptions[1].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 1);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_CLOCK);
      EXPECT_EQ(Events[0].userdata, 0x2020202020202020);
    };
    auto PollRead = [&]() {
      const uint32_t Count = 2;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_READ;
      Subscriptions[0].u.u.fd_read.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[1].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[1].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[1].u.u.clock.precision = 1;
      Subscriptions[1].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 1);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_FD_READ);
      EXPECT_EQ(Events[0].userdata, 0x1010101010101010);
      EXPECT_EQ(Events[0].fd_readwrite.flags, 0);
    };
    auto PollWriteTimeout = [&]() {
      const uint32_t Count = 2;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_WRITE;
      Subscriptions[0].u.u.fd_write.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[1].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[1].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[1].u.u.clock.precision = 1;
      Subscriptions[1].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 1);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_CLOCK);
      EXPECT_EQ(Events[0].userdata, 0x2020202020202020);
    };
    auto PollWrite = [&]() {
      const uint32_t Count = 2;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_WRITE;
      Subscriptions[0].u.u.fd_write.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[1].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[1].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[1].u.u.clock.precision = 1;
      Subscriptions[1].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 1);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_FD_WRITE);
      EXPECT_EQ(Events[0].userdata, 0x1010101010101010);
    };
    auto PollReadWriteTimeout = [&]() {
      const uint32_t Count = 3;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_READ;
      Subscriptions[0].u.u.fd_read.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_FD_WRITE;
      Subscriptions[1].u.u.fd_write.file_descriptor = Fd;
      Subscriptions[2].userdata = 0x3030303030303030;
      Subscriptions[2].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[2].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[2].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[2].u.u.clock.precision = 1;
      Subscriptions[2].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 1);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_CLOCK);
      EXPECT_EQ(Events[0].userdata, 0x3030303030303030);
    };
    auto PollReadWriteWrite = [&]() {
      const uint32_t Count = 3;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_READ;
      Subscriptions[0].u.u.fd_read.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_FD_WRITE;
      Subscriptions[1].u.u.fd_write.file_descriptor = Fd;
      Subscriptions[2].userdata = 0x3030303030303030;
      Subscriptions[2].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[2].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[2].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[2].u.u.clock.precision = 1;
      Subscriptions[2].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 1);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_FD_WRITE);
      EXPECT_EQ(Events[0].userdata, 0x2020202020202020);
    };
    auto PollReadWriteReadWrite = [&]() {
      const uint32_t Count = 3;
      const uint32_t NEventsPtr = 0;
      const uint32_t InPtr = NEventsPtr + sizeof(__wasi_size_t);
      const uint32_t OutPtr = InPtr + sizeof(__wasi_subscription_t) * Count;
      auto Subscriptions = MemInst.getPointer<__wasi_subscription_t *>(InPtr);
      Subscriptions[0].userdata = 0x1010101010101010;
      Subscriptions[0].u.tag = __WASI_EVENTTYPE_FD_READ;
      Subscriptions[0].u.u.fd_read.file_descriptor = Fd;
      Subscriptions[1].userdata = 0x2020202020202020;
      Subscriptions[1].u.tag = __WASI_EVENTTYPE_FD_WRITE;
      Subscriptions[1].u.u.fd_write.file_descriptor = Fd;
      Subscriptions[2].userdata = 0x3030303030303030;
      Subscriptions[2].u.tag = __WASI_EVENTTYPE_CLOCK;
      Subscriptions[2].u.u.clock.id = __WASI_CLOCKID_MONOTONIC;
      Subscriptions[2].u.u.clock.timeout =
          std::chrono::nanoseconds(std::chrono::milliseconds(100)).count();
      Subscriptions[2].u.u.clock.precision = 1;
      Subscriptions[2].u.u.clock.flags = static_cast<__wasi_subclockflags_t>(0);
      EXPECT_TRUE(WasiPollOneoff.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 4>{InPtr, OutPtr, Count, NEventsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr, sizeof(NEvents)));
      EXPECT_EQ(NEvents, 2);
      auto Events = MemInst.getPointer<__wasi_event_t *>(OutPtr);
      EXPECT_EQ(Events[0].type, __WASI_EVENTTYPE_FD_READ);
      EXPECT_EQ(Events[0].userdata, 0x1010101010101010);
      EXPECT_EQ(Events[1].type, __WASI_EVENTTYPE_FD_WRITE);
      EXPECT_EQ(Events[1].userdata, 0x2020202020202020);
    };

    // poll read and 100 milliseconds, expect timeout
    PollReadTimeout();

    // request server to send data
    Action.store(ServerAction::Send);
    ActionRequested.notify_one();
    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // poll read and 100 milliseconds, expect read event
    PollRead();

    // read data
    {
      const uint32_t IOVecSize = 1;
      const uint32_t NReadPtr = 0;
      const uint32_t RoFlagsPtr = NReadPtr + sizeof(__wasi_size_t);
      const uint32_t IOVecPtr = RoFlagsPtr + sizeof(__wasi_size_t);
      const uint32_t DataPtr = IOVecPtr + sizeof(__wasi_iovec_t) * IOVecSize;
      const uint32_t RiFlags = 0;
      auto *IOVec = MemInst.getPointer<__wasi_ciovec_t *>(
          IOVecPtr, sizeof(__wasi_ciovec_t) * IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = 256;
      EXPECT_TRUE(WasiSockRecv.run(
          &MemInst,
          std::array<WasmEdge::ValVariant, 6>{Fd, IOVecPtr, IOVecSize, RiFlags,
                                              NReadPtr, RoFlagsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NRead;
      EXPECT_TRUE(MemInst.loadValue(NRead, NReadPtr, sizeof(NRead)));
      EXPECT_EQ(NRead, "server"sv.size());
    }

    // poll read and 100 milliseconds, expect timeout
    PollReadTimeout();

    // set nonblock flag
    EXPECT_TRUE(WasiFdFdstatSetFlags.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 2>{
            Fd, static_cast<uint32_t>(__WASI_FDFLAGS_NONBLOCK)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    // write data until buffer full
    while (true) {
      const uint32_t IOVecSize = 1;
      const uint32_t NWrittenPtr = 0;
      const uint32_t RoFlagsPtr = NWrittenPtr + sizeof(__wasi_size_t);
      const uint32_t IOVecPtr = RoFlagsPtr + sizeof(__wasi_size_t);
      const uint32_t DataPtr = IOVecPtr + sizeof(__wasi_iovec_t) * IOVecSize;
      const uint32_t SiFlags = 0;
      const auto Data = "somedata"sv;
      writeString(MemInst, Data, DataPtr);
      auto *IOVec = MemInst.getPointer<__wasi_ciovec_t *>(
          IOVecPtr, sizeof(__wasi_ciovec_t) * IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = Data.size();
      EXPECT_TRUE(
          WasiSockSend.run(&MemInst,
                           std::array<WasmEdge::ValVariant, 5>{
                               Fd, IOVecPtr, IOVecSize, SiFlags, NWrittenPtr},
                           Errno));
      if (Errno[0].get<int32_t>() != __WASI_ERRNO_SUCCESS) {
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AGAIN);
        break;
      }
    }

    // poll write and 100 milliseconds, expect timeout
    PollWriteTimeout();

    // request server to recv data
    Action.store(ServerAction::Recv);
    ActionRequested.notify_one();
    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // poll write and 100 milliseconds, expect write
    PollWrite();

    // write data until buffer full
    while (true) {
      const uint32_t IOVecSize = 1;
      const uint32_t NWrittenPtr = 0;
      const uint32_t RoFlagsPtr = NWrittenPtr + sizeof(__wasi_size_t);
      const uint32_t IOVecPtr = RoFlagsPtr + sizeof(__wasi_size_t);
      const uint32_t DataPtr = IOVecPtr + sizeof(__wasi_iovec_t) * IOVecSize;
      const uint32_t SiFlags = 0;
      const auto Data = "somedata"sv;
      writeString(MemInst, Data, DataPtr);
      auto *IOVec = MemInst.getPointer<__wasi_ciovec_t *>(
          IOVecPtr, sizeof(__wasi_ciovec_t) * IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = Data.size();
      EXPECT_TRUE(
          WasiSockSend.run(&MemInst,
                           std::array<WasmEdge::ValVariant, 5>{
                               Fd, IOVecPtr, IOVecSize, SiFlags, NWrittenPtr},
                           Errno));
      if (Errno[0].get<int32_t>() != __WASI_ERRNO_SUCCESS) {
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AGAIN);
        break;
      }
    }

    // poll read, write and 100 milliseconds, expect timeout
    PollReadWriteTimeout();

    // request server to recv data
    Action.store(ServerAction::Recv);
    ActionRequested.notify_one();
    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // poll read, write and 100 milliseconds, expect write
    PollReadWriteWrite();

    // request server to send data
    Action.store(ServerAction::Send);
    ActionRequested.notify_one();
    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // poll read, write and 100 milliseconds, expect read and write
    PollReadWriteReadWrite();

    // close socket
    EXPECT_TRUE(WasiFdClose.run(
        &MemInst, std::array<WasmEdge::ValVariant, 1>{Fd}, Errno));
    Env.fini();
  }

  Action.store(ServerAction::Stop);
  ActionRequested.notify_one();
  Server.join();
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

TEST(WasiTest, GetAddrinfo) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::MemoryInstance MemInst(
      WasmEdge::AST::MemoryType(1));

  WasmEdge::Host::WasiGetAddrinfo WasiGetAddrinfo(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;

  uint32_t NodePtr = 0;
  uint32_t ServicePtr = 32;
  uint32_t HintsPtr = 48;
  uint32_t ResLengthPtr = 100;
  uint32_t ResultPtr = 104;
  std::string Node = "";
  std::string Service = "27015";
  uint32_t MaxLength = 10;
  uint32_t CanonnameMaxSize = 50;

  const uint32_t NodeLen = Node.size();
  const uint32_t ServiceLen = Service.size();

  __wasi_addrinfo_t Hints;
  std::memset(&Hints, 0, sizeof(Hints));
  Hints.ai_family = __WASI_ADDRESS_FAMILY_INET4;   /* Allow IPv4 */
  Hints.ai_socktype = __WASI_SOCK_TYPE_SOCK_DGRAM; /* Datagram socket */
  Hints.ai_flags = __WASI_AIFLAGS_AI_PASSIVE;      /* For wildcard IP address */
  writeString(MemInst, Node, NodePtr);
  writeString(MemInst, Service, ServicePtr);
  writeAddrinfo(MemInst, &Hints, HintsPtr);
  auto *ResLength =
      MemInst.getPointer<uint32_t *>(ResLengthPtr, sizeof(uint32_t));
  *ResLength = 0;
  auto *Result =
      MemInst.getPointer<uint8_t_ptr *>(ResultPtr, sizeof(uint8_t_ptr));
  *Result = 108;
  // allocate Res Item;
  allocateAddrinfoArray(MemInst, *Result, MaxLength, CanonnameMaxSize);

  Env.init({}, "test"s, {}, {});
  // MaxLength == 0;
  {
    uint32_t TmpResMaxLength = 0;
    EXPECT_TRUE(WasiGetAddrinfo.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 8>{NodePtr, NodeLen, ServicePtr,
                                            ServiceLen, HintsPtr, ResultPtr,
                                            TmpResMaxLength, ResLengthPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AIMEMORY);
  }
  // MemInst is nullptr
  {
    EXPECT_TRUE(WasiGetAddrinfo.run(
        nullptr,
        std::array<WasmEdge::ValVariant, 8>{NodePtr, NodeLen, ServicePtr,
                                            ServiceLen, HintsPtr, ResultPtr,
                                            MaxLength, ResLengthPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  }
  // Node and Service are all nullptr
  {
    uint32_t TmpNodeLen = 0;
    uint32_t TmpServiceLen = 0;
    EXPECT_TRUE(WasiGetAddrinfo.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 8>{NodePtr, TmpNodeLen, ServicePtr,
                                            TmpServiceLen, HintsPtr, ResultPtr,
                                            MaxLength, ResLengthPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AINONAME);
  }
  // node is nullptr, service is not nullptr
  {
    EXPECT_TRUE(WasiGetAddrinfo.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 8>{NodePtr, NodeLen, ServicePtr,
                                            ServiceLen, HintsPtr, ResultPtr,
                                            MaxLength, ResLengthPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    auto *Res =
        MemInst.getPointer<uint8_t_ptr *>(ResultPtr, sizeof(uint8_t_ptr));

    auto *ResHead = MemInst.getPointer<__wasi_addrinfo_t *>(
        *Res, sizeof(__wasi_addrinfo_t));
    auto *ResItem = ResHead;
    EXPECT_NE(*ResLength, 0);
    for (uint32_t Idx = 0; Idx < *ResLength; Idx++) {
      EXPECT_NE(ResItem->ai_addrlen, 0);
      auto *TmpSockAddr = MemInst.getPointer<__wasi_sockaddr_t *>(
          ResItem->ai_addr, sizeof(__wasi_sockaddr_t));
      EXPECT_EQ(TmpSockAddr->sa_data_len, 14);
      EXPECT_EQ(MemInst.getPointer<char *>(TmpSockAddr->sa_data,
                                           TmpSockAddr->sa_data_len)[0],
                'i');
      if (Idx != (*ResLength) - 1) {
        ResItem = MemInst.getPointer<__wasi_addrinfo_t *>(
            ResItem->ai_next, sizeof(__wasi_addrinfo_t));
      }
    }
  }
  allocateAddrinfoArray(MemInst, *Result, MaxLength, CanonnameMaxSize);
  // hints.ai_flag is ai_canonname but has an error
  {
    Hints.ai_flags = __WASI_AIFLAGS_AI_CANONNAME;
    writeAddrinfo(MemInst, &Hints, HintsPtr);
    EXPECT_TRUE(WasiGetAddrinfo.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 8>{NodePtr, NodeLen, ServicePtr,
                                            ServiceLen, HintsPtr, ResultPtr,
                                            MaxLength, ResLengthPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_AIBADFLAG);
  }

  // node is nullptr, service is not nullptr
  {
    std::string TmpNode = "google.com";
    writeString(MemInst, TmpNode, NodePtr);
    uint32_t TmpNodeLen = TmpNode.size();
    EXPECT_TRUE(WasiGetAddrinfo.run(
        &MemInst,
        std::array<WasmEdge::ValVariant, 8>{NodePtr, TmpNodeLen, ServicePtr,
                                            ServiceLen, HintsPtr, ResultPtr,
                                            MaxLength, ResLengthPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_NE(*ResLength, 0);
    auto *Res =
        MemInst.getPointer<uint8_t_ptr *>(ResultPtr, sizeof(uint8_t_ptr));

    auto *ResHead = MemInst.getPointer<__wasi_addrinfo_t *>(
        *Res, sizeof(__wasi_addrinfo_t));
    EXPECT_NE(ResHead->ai_canonname_len, 0);
    EXPECT_STREQ(MemInst.getPointer<const char *>(ResHead->ai_canonname,
                                                  ResHead->ai_canonname_len),
                 "google.com");
    auto *WasiSockAddr = MemInst.getPointer<__wasi_sockaddr_t *>(
        ResHead->ai_addr, sizeof(__wasi_sockaddr_t));
    EXPECT_EQ(WasiSockAddr->sa_data_len, 14);
  }
}
#endif

GTEST_API_ int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
