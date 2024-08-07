// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/defines.h"
#include "host/wasi/wasibase.h"
#include "host/wasi/wasifunc.h"
#include "runtime/instance/module.h"
#include "system/winapi.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <gtest/gtest.h>
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
  WasiAddress.buf_len = static_cast<__wasi_size_t>(Address.size());

  std::memcpy(MemInst.getPointer<__wasi_address_t *>(Ptr), &WasiAddress,
              sizeof(__wasi_address_t));
}

#if !WASMEDGE_OS_WINDOWS
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

__wasi_timestamp_t convertTimespec(const timespec &Timespec) noexcept {
  std::chrono::nanoseconds Time = std::chrono::seconds(Timespec.tv_sec);
  Time += std::chrono::nanoseconds(Timespec.tv_nsec);
  return static_cast<__wasi_timestamp_t>(Time.count());
}
#else

__wasi_timestamp_t
convertFiletime(WasmEdge::winapi::FILETIME_ FileTime) noexcept {
  using std::chrono::duration_cast;
  using std::chrono::nanoseconds;
  using FiletimeDuration = std::chrono::duration<
      uint64_t, std::ratio_multiply<std::ratio<100, 1>,
                                    std::chrono::nanoseconds::period>>;
  /// from 1601-01-01 to 1970-01-01, 134774 days
  constexpr const FiletimeDuration NTToUnixEpoch =
      std::chrono::seconds{134774LL * 86400LL};
  WasmEdge::winapi::ULARGE_INTEGER_ Temp = {
      /* LowPart */ FileTime.dwLowDateTime,
      /* HighPart */ FileTime.dwHighDateTime};
  auto Duration = duration_cast<nanoseconds>(FiletimeDuration{Temp.QuadPart} -
                                             NTToUnixEpoch);
  return static_cast<__wasi_timestamp_t>(Duration.count());
}
#endif

// The following code includes a sleep to prevent a possible delay when sending
// and recving data. There is a chance that PollOneoff may not immediately get
// the read event when it is called right after the server has sent the data.
// Without the sleep, there is a risk that the unit test may not pass. We found
// this problem on macOS and Windows.
void sleepForMacWin() noexcept {
#if WASMEDGE_OS_MACOS || WASMEDGE_OS_WINDOWS
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
#endif
}
} // namespace

TEST(WasiTest, Args) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiArgsSizesGet WasiArgsSizesGet(Env);
  WasmEdge::Host::WasiArgsGet WasiArgsGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  // args: test\0
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(5));

  EXPECT_TRUE(WasiArgsGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(4));
  EXPECT_STREQ(MemInst.getPointer<const char *>(4), "test");
  Env.fini();

  // args: test\0 abc\0
  Env.init({}, "test"s, {"abc"s}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(9));

  EXPECT_TRUE(WasiArgsGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(13));
  EXPECT_STREQ(MemInst.getPointer<const char *>(8), "test");
  EXPECT_STREQ(MemInst.getPointer<const char *>(13), "abc");
  Env.fini();

  // args: test\0 \0
  Env.init({}, "test"s, {""s}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(6));

  EXPECT_TRUE(WasiArgsGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(8));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(13));
  EXPECT_STREQ(MemInst.getPointer<const char *>(8), "test");
  EXPECT_STREQ(MemInst.getPointer<const char *>(13), "");
  Env.fini();

  // invalid pointer
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiArgsSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiArgsSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiArgsGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiArgsGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();
}

TEST(WasiTest, Envs) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiEnvironSizesGet WasiEnvironSizesGet(Env);
  WasmEdge::Host::WasiEnvironGet WasiEnvironGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  // envs:
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0));

  *MemInst.getPointer<uint32_t *>(0) = UINT32_C(0xdeadbeef);
  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(0)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xdeadbeef));
  Env.fini();

  // envs: a=b\0
  Env.init({}, "test"s, {}, {"a=b"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(1));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(4));

  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(4));
  EXPECT_STREQ(MemInst.getPointer<const char *>(4), "a=b");
  Env.fini();

  // envs: a=b\0 TEST=TEST=Test\0
  Env.init({}, "test"s, {}, {"a=b"s, "TEST=TEST=TEST"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(2));
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(19));

  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(12)},
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
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  // success on zero-size write
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  // success on zero-size write
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();

  Env.init({}, "test"s, {}, {"a=b"s});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(4)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(4), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironSizesGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));

  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(8)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(8), UINT32_C(0xa5a5a5a5));
  EXPECT_TRUE(WasiEnvironGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(65536)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();
}

TEST(WasiTest, ClockRes) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiClockResGet WasiClockResGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  Env.init({}, "test"s, {}, {});
#if !WASMEDGE_OS_WINDOWS
  // realtime clock
  {
    timespec Timespec;
    int SysErrno = 0;
    if (clock_getres(CLOCK_REALTIME, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
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
    timespec Timespec;
    int SysErrno = 0;
    if (clock_getres(CLOCK_MONOTONIC, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
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
    timespec Timespec;
    int SysErrno = 0;
    if (clock_getres(CLOCK_PROCESS_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
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
    timespec Timespec;
    int SysErrno = 0;
    if (clock_getres(CLOCK_THREAD_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_CLOCKID_THREAD_CPUTIME_ID),
            UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Res = convertTimespec(Timespec);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Res);
    }
  }
#else
  // monotonic clock
  {
    WasmEdge::winapi::LARGE_INTEGER_ Frequency;
    WasmEdge::winapi::QueryPerformanceFrequency(&Frequency);
    const std::chrono::nanoseconds Result =
        std::chrono::nanoseconds(std::chrono::seconds{1}) /
        static_cast<uint64_t>(Frequency.QuadPart);
    const uint64_t Resolution = static_cast<uint64_t>(Result.count());

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_CLOCKID_MONOTONIC), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Resolution);
  }

  // other clock
  {
    using FiletimeDuration = std::chrono::duration<
        uint64_t, std::ratio_multiply<std::ratio<100, 1>,
                                      std::chrono::nanoseconds::period>>;
    WasmEdge::winapi::ULONG_ MinimumResolution;
    WasmEdge::winapi::ULONG_ MaximumResolution;
    WasmEdge::winapi::ULONG_ CurrentResolution;
    EXPECT_TRUE(
        WasmEdge::winapi::NT_SUCCESS_(WasmEdge::winapi::NtQueryTimerResolution(
            &MinimumResolution, &MaximumResolution, &CurrentResolution)));
    const std::chrono::nanoseconds Result = FiletimeDuration{CurrentResolution};
    const uint64_t Resolution = static_cast<uint64_t>(Result.count());
    for (const auto ClockId :
         {__WASI_CLOCKID_REALTIME, __WASI_CLOCKID_PROCESS_CPUTIME_ID,
          __WASI_CLOCKID_THREAD_CPUTIME_ID}) {
      writeDummyMemoryContent(MemInst);
      EXPECT_TRUE(
          WasiClockResGet.run(CallFrame,
                              std::initializer_list<WasmEdge::ValVariant>{
                                  static_cast<uint32_t>(ClockId), UINT32_C(0)},
                              Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      EXPECT_EQ(*MemInst.getPointer<const uint64_t *>(0), Resolution);
    }
  }
#endif

  // invalid clockid
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(4), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
  }

  // invalid pointer
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockResGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_CLOCKID_REALTIME), UINT32_C(65536)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  }

  Env.fini();
}

TEST(WasiTest, PollOneoffSocketV1) {
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
    WasmEdge::Runtime::Instance::ModuleInstance Mod("");
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(1)));
    auto *MemInstPtr = Mod.findMemoryExports("memory");
    ASSERT_TRUE(MemInstPtr != nullptr);
    auto &MemInst = *MemInstPtr;
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

    WasmEdge::Host::WasiFdClose WasiFdClose(Env);
    WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
    WasmEdge::Host::WasiSockAcceptV1 WasiSockAccept(Env);
    WasmEdge::Host::WasiSockBindV1 WasiSockBind(Env);
    WasmEdge::Host::WasiSockListenV1 WasiSockListen(Env);
    WasmEdge::Host::WasiSockOpenV1 WasiSockOpen(Env);
    WasmEdge::Host::WasiSockRecvV1 WasiSockRecv(Env);
    WasmEdge::Host::WasiSockSendV1 WasiSockSend(Env);
    WasmEdge::Host::WasiSockSetOpt WasiSockSetOpt(Env);

    std::array<WasmEdge::ValVariant, 1> Errno;
    const uint32_t FdPtr = 0;
    const uint32_t AddressPtr = 4;
    const int32_t Backlog = 1;
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
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ConnectionFd}, Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        Env.fini();
        return;
      }
      case ServerAction::Start: {
        int32_t ServerFd = -1;
        // open socket
        EXPECT_TRUE(WasiSockOpen.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
                static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE((MemInst.loadValue(ServerFd, FdPtr)));

        // set socket options
        const uint32_t SockOptionsPtr = 0;
        const uint32_t One = 1;
        MemInst.storeValue(One, SockOptionsPtr);
        EXPECT_TRUE(WasiSockSetOpt.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                ServerFd,
                static_cast<uint32_t>(__WASI_SOCK_OPT_LEVEL_SOL_SOCKET),
                static_cast<uint32_t>(__WASI_SOCK_OPT_SO_REUSEADDR),
                static_cast<uint32_t>(SockOptionsPtr),
                static_cast<uint32_t>(sizeof(One))},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // bind port
        writeAddress(MemInst, Address, AddressPtr);
        EXPECT_TRUE(
            WasiSockBind.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 ServerFd, AddressPtr, Port},
                             Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // listen port
        EXPECT_TRUE(WasiSockListen.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ServerFd, Backlog},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        ActionDone.store(true);
        ActionProcessed.notify_one();

        // accept port
        EXPECT_TRUE(WasiSockAccept.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ServerFd, FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE((MemInst.loadValue(ConnectionFd, FdPtr)));

        // close socket
        EXPECT_TRUE(WasiFdClose.run(
            CallFrame, std::initializer_list<WasmEdge::ValVariant>{ServerFd},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // set nonblock flag
        EXPECT_TRUE(WasiFdFdstatSetFlags.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
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
        auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
        IOVec[0].buf = DataPtr;
        IOVec[0].buf_len = static_cast<__wasi_size_t>(Data.size());
        EXPECT_TRUE(WasiSockSend.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                ConnectionFd, IOVecPtr, IOVecSize, SiFlags, NWrittenPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        __wasi_size_t NWritten;
        EXPECT_TRUE((MemInst.loadValue(NWritten, NWrittenPtr)));
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
          auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
          IOVec[0].buf = DataPtr;
          IOVec[0].buf_len = 32768;
          EXPECT_TRUE(
              WasiSockRecv.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ConnectionFd, IOVecPtr, IOVecSize, RiFlags,
                                   NReadPtr, RoFlagsPtr},
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
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
  WasmEdge::Host::WasiPollOneoff<WasmEdge::Host::WASI::TriggerType::Level>
      WasiPollOneoff(Env);
  WasmEdge::Host::WasiSockConnectV1 WasiSockConnect(Env);
  WasmEdge::Host::WasiSockOpenV1 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockRecvV1 WasiSockRecv(Env);
  WasmEdge::Host::WasiSockSendV1 WasiSockSend(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  const uint32_t FdPtr = 0;
  const uint32_t AddressPtr = 4;

  {
    Env.init({}, "test"s, {}, {});

    // open socket
    EXPECT_TRUE(WasiSockOpen.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
            static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    int32_t Fd;
    EXPECT_TRUE((MemInst.loadValue(Fd, FdPtr)));

    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // connect server
    writeAddress(MemInst, Address, AddressPtr);
    EXPECT_TRUE(WasiSockConnect.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, AddressPtr, Port},
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = 256;
      EXPECT_TRUE(WasiSockRecv.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{
              Fd, IOVecPtr, IOVecSize, RiFlags, NReadPtr, RoFlagsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NRead;
      EXPECT_TRUE(MemInst.loadValue(NRead, NReadPtr));
      EXPECT_EQ(NRead, "server"sv.size());
    }

    // poll read and 100 milliseconds, expect timeout
    PollReadTimeout();

    // set nonblock flag
    EXPECT_TRUE(WasiFdFdstatSetFlags.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = static_cast<__wasi_size_t>(Data.size());
      EXPECT_TRUE(
          WasiSockSend.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = static_cast<__wasi_size_t>(Data.size());
      EXPECT_TRUE(
          WasiSockSend.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
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

    sleepForMacWin();

    // poll read, write and 100 milliseconds, expect read and write
    PollReadWriteReadWrite();

    // close socket
    EXPECT_TRUE(WasiFdClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{Fd}, Errno));
    Env.fini();
  }

  Action.store(ServerAction::Stop);
  ActionRequested.notify_one();
  Server.join();
}

TEST(WasiTest, PollOneoffSocketV2) {
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
  const std::array<uint8_t, 128> Address{1, 0, 127, 0, 0, 1};
  const uint32_t Port = 18000;

  std::thread Server([&]() {
    WasmEdge::Host::WASI::Environ Env;
    WasmEdge::Runtime::Instance::ModuleInstance Mod("");
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(1)));
    auto *MemInstPtr = Mod.findMemoryExports("memory");
    ASSERT_TRUE(MemInstPtr != nullptr);
    auto &MemInst = *MemInstPtr;
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

    WasmEdge::Host::WasiFdClose WasiFdClose(Env);
    WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
    WasmEdge::Host::WasiSockAcceptV2 WasiSockAccept(Env);
    WasmEdge::Host::WasiSockBindV2 WasiSockBind(Env);
    WasmEdge::Host::WasiSockListenV2 WasiSockListen(Env);
    WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
    WasmEdge::Host::WasiSockRecvV2 WasiSockRecv(Env);
    WasmEdge::Host::WasiSockSendV2 WasiSockSend(Env);
    WasmEdge::Host::WasiSockSetOpt WasiSockSetOpt(Env);

    std::array<WasmEdge::ValVariant, 1> Errno;
    const uint32_t FdPtr = 0;
    const uint32_t AddressPtr = 4;
    const int32_t Backlog = 1;
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
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ConnectionFd}, Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        Env.fini();
        return;
      }
      case ServerAction::Start: {
        int32_t ServerFd = -1;
        // open socket
        EXPECT_TRUE(WasiSockOpen.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
                static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE((MemInst.loadValue(ServerFd, FdPtr)));

        // set socket options
        const uint32_t SockOptionsPtr = 0;
        const uint32_t One = 1;
        MemInst.storeValue(One, SockOptionsPtr);
        EXPECT_TRUE(WasiSockSetOpt.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                ServerFd,
                static_cast<uint32_t>(__WASI_SOCK_OPT_LEVEL_SOL_SOCKET),
                static_cast<uint32_t>(__WASI_SOCK_OPT_SO_REUSEADDR),
                static_cast<uint32_t>(SockOptionsPtr),
                static_cast<uint32_t>(sizeof(One))},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // bind port
        writeAddress(MemInst, Address, AddressPtr);
        EXPECT_TRUE(
            WasiSockBind.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 ServerFd, AddressPtr, Port},
                             Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // listen port
        EXPECT_TRUE(WasiSockListen.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ServerFd, Backlog},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        ActionDone.store(true);
        ActionProcessed.notify_one();

        // accept port
        EXPECT_TRUE(
            WasiSockAccept.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ServerFd, UINT32_C(0), FdPtr},
                               Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE((MemInst.loadValue(ConnectionFd, FdPtr)));

        // close socket
        EXPECT_TRUE(WasiFdClose.run(
            CallFrame, std::initializer_list<WasmEdge::ValVariant>{ServerFd},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // set nonblock flag
        EXPECT_TRUE(WasiFdFdstatSetFlags.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
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
        auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
        IOVec[0].buf = DataPtr;
        IOVec[0].buf_len = static_cast<__wasi_size_t>(Data.size());
        EXPECT_TRUE(WasiSockSend.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                ConnectionFd, IOVecPtr, IOVecSize, SiFlags, NWrittenPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        __wasi_size_t NWritten;
        EXPECT_TRUE((MemInst.loadValue(NWritten, NWrittenPtr)));
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
          auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
          IOVec[0].buf = DataPtr;
          IOVec[0].buf_len = 32768;
          EXPECT_TRUE(
              WasiSockRecv.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ConnectionFd, IOVecPtr, IOVecSize, RiFlags,
                                   NReadPtr, RoFlagsPtr},
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
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
  WasmEdge::Host::WasiPollOneoff<WasmEdge::Host::WASI::TriggerType::Level>
      WasiPollOneoff(Env);
  WasmEdge::Host::WasiSockConnectV2 WasiSockConnect(Env);
  WasmEdge::Host::WasiSockOpenV2 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockRecvV2 WasiSockRecv(Env);
  WasmEdge::Host::WasiSockSendV2 WasiSockSend(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  const uint32_t FdPtr = 0;
  const uint32_t AddressPtr = 4;

  {
    Env.init({}, "test"s, {}, {});

    // open socket
    EXPECT_TRUE(WasiSockOpen.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
            static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    int32_t Fd;
    EXPECT_TRUE((MemInst.loadValue(Fd, FdPtr)));

    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // connect server
    writeAddress(MemInst, Address, AddressPtr);
    EXPECT_TRUE(WasiSockConnect.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, AddressPtr, Port},
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = 256;
      EXPECT_TRUE(WasiSockRecv.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{
              Fd, IOVecPtr, IOVecSize, RiFlags, NReadPtr, RoFlagsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NRead;
      EXPECT_TRUE(MemInst.loadValue(NRead, NReadPtr));
      EXPECT_EQ(NRead, "server"sv.size());
    }

    // poll read and 100 milliseconds, expect timeout
    PollReadTimeout();

    // set nonblock flag
    EXPECT_TRUE(WasiFdFdstatSetFlags.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = static_cast<__wasi_size_t>(Data.size());
      EXPECT_TRUE(
          WasiSockSend.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = static_cast<__wasi_size_t>(Data.size());
      EXPECT_TRUE(
          WasiSockSend.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
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

    sleepForMacWin();

    // poll read, write and 100 milliseconds, expect read and write
    PollReadWriteReadWrite();

    // close socket
    EXPECT_TRUE(WasiFdClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{Fd}, Errno));
    Env.fini();
  }

  Action.store(ServerAction::Stop);
  ActionRequested.notify_one();
  Server.join();
}

#if !WASMEDGE_OS_WINDOWS
TEST(WasiTest, EpollOneoffSocketV1) {
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
    WasmEdge::Runtime::Instance::ModuleInstance Mod("");
    Mod.addHostMemory(
        "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                      WasmEdge::AST::MemoryType(1)));
    auto *MemInstPtr = Mod.findMemoryExports("memory");
    ASSERT_TRUE(MemInstPtr != nullptr);
    auto &MemInst = *MemInstPtr;
    WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

    WasmEdge::Host::WasiFdClose WasiFdClose(Env);
    WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
    WasmEdge::Host::WasiSockAcceptV1 WasiSockAccept(Env);
    WasmEdge::Host::WasiSockBindV1 WasiSockBind(Env);
    WasmEdge::Host::WasiSockListenV1 WasiSockListen(Env);
    WasmEdge::Host::WasiSockOpenV1 WasiSockOpen(Env);
    WasmEdge::Host::WasiSockRecvV1 WasiSockRecv(Env);
    WasmEdge::Host::WasiSockSendV1 WasiSockSend(Env);
    WasmEdge::Host::WasiSockSetOpt WasiSockSetOpt(Env);

    std::array<WasmEdge::ValVariant, 1> Errno;
    const uint32_t FdPtr = 0;
    const uint32_t AddressPtr = 4;
    const int32_t Backlog = 1;
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
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ConnectionFd}, Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        Env.fini();
        return;
      }
      case ServerAction::Start: {
        int32_t ServerFd = -1;
        // open socket
        EXPECT_TRUE(WasiSockOpen.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
                static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE((MemInst.loadValue(ServerFd, FdPtr)));

        // set socket options
        const uint32_t SockOptionsPtr = 0;
        const uint32_t One = 1;
        MemInst.storeValue(One, SockOptionsPtr);
        EXPECT_TRUE(WasiSockSetOpt.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                ServerFd,
                static_cast<uint32_t>(__WASI_SOCK_OPT_LEVEL_SOL_SOCKET),
                static_cast<uint32_t>(__WASI_SOCK_OPT_SO_REUSEADDR),
                static_cast<uint32_t>(SockOptionsPtr),
                static_cast<uint32_t>(sizeof(One))},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // bind port
        writeAddress(MemInst, Address, AddressPtr);
        EXPECT_TRUE(
            WasiSockBind.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 ServerFd, AddressPtr, Port},
                             Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // listen port
        EXPECT_TRUE(WasiSockListen.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ServerFd, Backlog},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        ActionDone.store(true);
        ActionProcessed.notify_one();

        // accept port
        EXPECT_TRUE(WasiSockAccept.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{ServerFd, FdPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        EXPECT_TRUE((MemInst.loadValue(ConnectionFd, FdPtr)));

        // close socket
        EXPECT_TRUE(WasiFdClose.run(
            CallFrame, std::initializer_list<WasmEdge::ValVariant>{ServerFd},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

        // set nonblock flag
        EXPECT_TRUE(WasiFdFdstatSetFlags.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
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
        auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
        IOVec[0].buf = DataPtr;
        IOVec[0].buf_len = Data.size();
        EXPECT_TRUE(WasiSockSend.run(
            CallFrame,
            std::initializer_list<WasmEdge::ValVariant>{
                ConnectionFd, IOVecPtr, IOVecSize, SiFlags, NWrittenPtr},
            Errno));
        EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
        __wasi_size_t NWritten;
        EXPECT_TRUE((MemInst.loadValue(NWritten, NWrittenPtr)));
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
          auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
          IOVec[0].buf = DataPtr;
          IOVec[0].buf_len = 32768;
          EXPECT_TRUE(
              WasiSockRecv.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   ConnectionFd, IOVecPtr, IOVecSize, RiFlags,
                                   NReadPtr, RoFlagsPtr},
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
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiFdClose WasiFdClose(Env);
  WasmEdge::Host::WasiFdFdstatSetFlags WasiFdFdstatSetFlags(Env);
  WasmEdge::Host::WasiPollOneoff<WasmEdge::Host::WASI::TriggerType::Edge>
      WasiPollOneoff(Env);
  WasmEdge::Host::WasiSockConnectV1 WasiSockConnect(Env);
  WasmEdge::Host::WasiSockOpenV1 WasiSockOpen(Env);
  WasmEdge::Host::WasiSockRecvV1 WasiSockRecv(Env);
  WasmEdge::Host::WasiSockSendV1 WasiSockSend(Env);

  std::array<WasmEdge::ValVariant, 1> Errno;
  const uint32_t FdPtr = 0;
  const uint32_t AddressPtr = 4;

  {
    Env.init({}, "test"s, {}, {});

    // open socket
    EXPECT_TRUE(WasiSockOpen.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_ADDRESS_FAMILY_INET4),
            static_cast<uint32_t>(__WASI_SOCK_TYPE_SOCK_STREAM), FdPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    int32_t Fd;
    EXPECT_TRUE((MemInst.loadValue(Fd, FdPtr)));

    {
      std::unique_lock<std::mutex> Lock(Mutex);
      ActionProcessed.wait(Lock, [&]() { return ActionDone.exchange(false); });
    }

    // connect server
    writeAddress(MemInst, Address, AddressPtr);
    EXPECT_TRUE(WasiSockConnect.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, AddressPtr, Port},
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE((MemInst.loadValue(NEvents, NEventsPtr)));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      EXPECT_TRUE(
          WasiPollOneoff.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 InPtr, OutPtr, Count, NEventsPtr},
                             Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NEvents;
      EXPECT_TRUE(MemInst.loadValue(NEvents, NEventsPtr));
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = 256;
      EXPECT_TRUE(WasiSockRecv.run(
          CallFrame,
          std::initializer_list<WasmEdge::ValVariant>{
              Fd, IOVecPtr, IOVecSize, RiFlags, NReadPtr, RoFlagsPtr},
          Errno));
      EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
      __wasi_size_t NRead;
      EXPECT_TRUE(MemInst.loadValue(NRead, NReadPtr));
      EXPECT_EQ(NRead, "server"sv.size());
    }

    // poll read and 100 milliseconds, expect timeout
    PollReadTimeout();

    // set nonblock flag
    EXPECT_TRUE(WasiFdFdstatSetFlags.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = Data.size();
      EXPECT_TRUE(
          WasiSockSend.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
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
      auto IOVec = MemInst.getSpan<__wasi_ciovec_t>(IOVecPtr, IOVecSize);
      IOVec[0].buf = DataPtr;
      IOVec[0].buf_len = Data.size();
      EXPECT_TRUE(
          WasiSockSend.run(CallFrame,
                           std::initializer_list<WasmEdge::ValVariant>{
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

    sleepForMacWin();

    // poll read, write and 100 milliseconds, expect read and write
    PollReadWriteReadWrite();

    // close socket
    EXPECT_TRUE(WasiFdClose.run(
        CallFrame, std::initializer_list<WasmEdge::ValVariant>{Fd}, Errno));
    Env.fini();
  }

  Action.store(ServerAction::Stop);
  ActionRequested.notify_one();
  Server.join();
}
#endif

TEST(WasiTest, ClockTimeGet) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiClockTimeGet WasiClockTimeGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  Env.init({}, "test"s, {}, {});

  // realtime clock
#if !WASMEDGE_OS_WINDOWS
  {
    timespec Timespec;
    int SysErrno = 0;
    if (clock_gettime(CLOCK_REALTIME, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(
        WasiClockTimeGet.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 static_cast<uint32_t>(__WASI_CLOCKID_REALTIME),
                                 UINT64_C(0), UINT32_C(0)},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }
#else
  {
    WasmEdge::winapi::FILETIME_ SysNow;
#if NTDDI_VERSION >= NTDDI_WIN8
    WasmEdge::winapi::GetSystemTimePreciseAsFileTime(&SysNow);
#else
    WasmEdge::winapi::GetSystemTimeAsFileTime(&SysNow);
#endif

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(
        WasiClockTimeGet.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 static_cast<uint32_t>(__WASI_CLOCKID_REALTIME),
                                 UINT64_C(0), UINT32_C(0)},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    const uint64_t Time = convertFiletime(SysNow);
    EXPECT_NEAR(static_cast<double>(*MemInst.getPointer<const uint64_t *>(0)),
                static_cast<double>(Time), 1000000.0);
  }
#endif

#if !WASMEDGE_OS_WINDOWS
  // monotonic clock
  {
    timespec Timespec;
    int SysErrno = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_CLOCKID_MONOTONIC), UINT64_C(0),
            UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }
#else
// TODO: implement
#endif

#if !WASMEDGE_OS_WINDOWS
  // process cputime clock
  {
    timespec Timespec;
    int SysErrno = 0;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_CLOCKID_PROCESS_CPUTIME_ID),
            UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }
#else
// TODO: implement
#endif

#if !WASMEDGE_OS_WINDOWS
  // thread cputime clock
  {
    timespec Timespec;
    int SysErrno = 0;
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &Timespec) != 0) {
      SysErrno = errno;
    }

    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiClockTimeGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            static_cast<uint32_t>(__WASI_CLOCKID_THREAD_CPUTIME_ID),
            UINT64_C(0), UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), convertErrno(SysErrno));
    if (SysErrno == 0) {
      const uint64_t Time = convertTimespec(Timespec);
      EXPECT_NEAR(*MemInst.getPointer<const uint64_t *>(0), Time, 1000000);
    }
  }
#else
// TODO: implement
#endif

  // invalid clockid
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(
        WasiClockTimeGet.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 UINT32_C(4), UINT64_C(0), UINT32_C(0)},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_INVAL);
  }

  // invalid pointer
  {
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(
        WasiClockTimeGet.run(CallFrame,
                             std::initializer_list<WasmEdge::ValVariant>{
                                 static_cast<uint32_t>(__WASI_CLOCKID_REALTIME),
                                 UINT64_C(0), UINT32_C(65536)},
                             Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  }

  Env.fini();
}

TEST(WasiTest, ProcExit) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiProcExit WasiProcExit(Env);

  Env.init({}, "test"s, {}, {});
  EXPECT_FALSE(WasiProcExit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0)}, {}));
  EXPECT_EQ(Env.getExitCode(), INT32_C(0));
  Env.fini();

  Env.init({}, "test"s, {}, {});
  EXPECT_FALSE(WasiProcExit.run(
      CallFrame, std::initializer_list<WasmEdge::ValVariant>{UINT32_C(1)}, {}));
  EXPECT_EQ(Env.getExitCode(), INT32_C(1));
  Env.fini();
}

TEST(WasiTest, Random) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiRandomGet WasiRandomGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno;

  // valid pointer, zero size
  Env.init({}, "test"s, {}, {});
  writeDummyMemoryContent(MemInst);
  EXPECT_TRUE(WasiRandomGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(0)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  EXPECT_EQ(*MemInst.getPointer<const uint32_t *>(0), UINT32_C(0xa5a5a5a5));
  Env.fini();

  // valid pointer, size 1
  {
    Env.init({}, "test"s, {}, {});
    writeDummyMemoryContent(MemInst);
    EXPECT_TRUE(WasiRandomGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(1)},
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
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{UINT32_C(0), UINT32_C(8)},
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
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(0)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
  Env.fini();

  // invalid pointer, non zero size
  Env.init({}, "test"s, {}, {});
  EXPECT_TRUE(WasiRandomGet.run(
      CallFrame,
      std::initializer_list<WasmEdge::ValVariant>{UINT32_C(65536), UINT32_C(1)},
      Errno));
  EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
  Env.fini();
}

TEST(WasiTest, Directory) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiPathCreateDirectory WasiPathCreateDirectory(Env);
  WasmEdge::Host::WasiPathRemoveDirectory WasiPathRemoveDirectory(Env);
  WasmEdge::Host::WasiPathFilestatGet WasiPathFilestatGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  const uint32_t Fd = 3;
  uint32_t PathPtr = 65536;

  // invalid pointer, zero size
  {
    Env.init({"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, PathPtr, UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // invalid pointer, non zero size
  {
    Env.init({"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, PathPtr, UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    Env.fini();
  }

  PathPtr = 0;
  // zero size path
  {
    Env.init({"/:."s}, "test"s, {}, {});
    const auto Path = ""sv;
    const uint32_t PathSize = static_cast<uint32_t>(Path.size());
    writeString(MemInst, Path, PathPtr);
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // exists directory
  {
    Env.init({"/:."s}, "test"s, {}, {});
    const auto Path = "."sv;
    const uint32_t PathSize = static_cast<uint32_t>(Path.size());
    writeString(MemInst, Path, PathPtr);
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_EXIST);
    Env.fini();
  }

  // create directory, check type and remove normal directory
  {
    Env.init({"/:."s}, "test"s, {}, {});
    const auto Path = "tmp"sv;
    const uint32_t PathSize = static_cast<uint32_t>(Path.size());
    writeString(MemInst, Path, PathPtr);
    EXPECT_TRUE(WasiPathCreateDirectory.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const uint32_t FilestatPtr = 8;
    EXPECT_TRUE(WasiPathFilestatGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            Fd, static_cast<uint32_t>(__WASI_LOOKUPFLAGS_SYMLINK_FOLLOW),
            PathPtr, PathSize, FilestatPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    const auto &Filestat =
        *MemInst.getPointer<const __wasi_filestat_t *>(FilestatPtr);
    EXPECT_EQ(Filestat.filetype, __WASI_FILETYPE_DIRECTORY);

    EXPECT_TRUE(WasiPathRemoveDirectory.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{Fd, PathPtr, PathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    Env.fini();
  }
}

#if !WASMEDGE_OS_WINDOWS
TEST(WasiTest, SymbolicLink) {
  WasmEdge::Host::WASI::Environ Env;
  WasmEdge::Runtime::Instance::ModuleInstance Mod("");
  Mod.addHostMemory(
      "memory", std::make_unique<WasmEdge::Runtime::Instance::MemoryInstance>(
                    WasmEdge::AST::MemoryType(1)));
  auto *MemInstPtr = Mod.findMemoryExports("memory");
  ASSERT_TRUE(MemInstPtr != nullptr);
  auto &MemInst = *MemInstPtr;
  WasmEdge::Runtime::CallingFrame CallFrame(nullptr, &Mod);

  WasmEdge::Host::WasiPathSymlink WasiPathSymlink(Env);
  WasmEdge::Host::WasiPathUnlinkFile WasiPathUnlinkFile(Env);
  WasmEdge::Host::WasiPathFilestatGet WasiPathFilestatGet(Env);
  std::array<WasmEdge::ValVariant, 1> Errno = {UINT32_C(0)};

  const uint32_t Fd = 3;
  uint32_t OldPathPtr = 65536;
  uint32_t NewPathPtr = 65552;

  // invalid pointer, zero size
  {
    Env.init({"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, UINT32_C(0), Fd,
                                                    NewPathPtr, UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // invalid pointer, non zero size
  {
    Env.init({"/:."s}, "test"s, {}, {});
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, UINT32_C(0), Fd,
                                                    NewPathPtr, UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, UINT32_C(1), Fd,
                                                    NewPathPtr, UINT32_C(0)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, UINT32_C(1), Fd,
                                                    NewPathPtr, UINT32_C(1)},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_FAULT);
    Env.fini();
  }

  OldPathPtr = 0;
  NewPathPtr = 16;
  // zero size path
  {
    Env.init({"/:."s}, "test"s, {}, {});
    const auto OldPath = ""sv;
    const auto NewPath = ""sv;
    const uint32_t OldPathSize = OldPath.size();
    const uint32_t NewPathSize = NewPath.size();
    writeString(MemInst, OldPath, OldPathPtr);
    writeString(MemInst, NewPath, NewPathPtr);
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, OldPathSize, Fd,
                                                    NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_NOENT);
    Env.fini();
  }

  // exists file
  {
    Env.init({"/:."s}, "test"s, {}, {});
    const auto OldPath = "."sv;
    const auto NewPath = "."sv;
    const uint32_t OldPathSize = OldPath.size();
    const uint32_t NewPathSize = NewPath.size();
    writeString(MemInst, OldPath, OldPathPtr);
    writeString(MemInst, NewPath, NewPathPtr);
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, OldPathSize, Fd,
                                                    NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_EXIST);
    Env.fini();
  }

  // create symbolic link, check type and remove normal symbolic link
  {
    Env.init({"/:."s}, "test"s, {}, {});
    const auto OldPath = "."sv;
    const auto NewPath = "tmp"sv;
    const uint32_t OldPathSize = OldPath.size();
    const uint32_t NewPathSize = NewPath.size();
    writeString(MemInst, OldPath, OldPathPtr);
    writeString(MemInst, NewPath, NewPathPtr);
    EXPECT_TRUE(WasiPathSymlink.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{OldPathPtr, OldPathSize, Fd,
                                                    NewPathPtr, NewPathSize},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);

    const uint32_t FilestatPtr = 32;
    const auto &Filestat =
        *MemInst.getPointer<const __wasi_filestat_t *>(FilestatPtr);

    EXPECT_TRUE(WasiPathFilestatGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            Fd, static_cast<uint32_t>(0), NewPathPtr, NewPathSize, FilestatPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(Filestat.filetype, __WASI_FILETYPE_SYMBOLIC_LINK);

    EXPECT_TRUE(WasiPathFilestatGet.run(
        CallFrame,
        std::initializer_list<WasmEdge::ValVariant>{
            Fd, static_cast<uint32_t>(__WASI_LOOKUPFLAGS_SYMLINK_FOLLOW),
            NewPathPtr, NewPathSize, FilestatPtr},
        Errno));
    EXPECT_EQ(Errno[0].get<int32_t>(), __WASI_ERRNO_SUCCESS);
    EXPECT_EQ(Filestat.filetype, __WASI_FILETYPE_DIRECTORY);

    EXPECT_TRUE(
        WasiPathUnlinkFile.run(CallFrame,
                               std::initializer_list<WasmEdge::ValVariant>{
                                   Fd, NewPathPtr, NewPathSize},
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
